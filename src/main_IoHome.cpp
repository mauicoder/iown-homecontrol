#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "IoHome.h" // Include the IoHome library header
#include <Wire.h>
#include <U8g2lib.h>

// --- HELTEC V3.2 PIN MAPPING ---
#define HW_VEXT            36   // Power Rail (Active LOW)
#define HW_LED             35   // System LED
#define LORA_NSS           8    // Chip Select
#define LORA_DIO1          14   // Interrupt
#define LORA_NRST          12   // Reset
#define LORA_BUSY          13   // Busy Pin
#define LORA_SCK           9
#define LORA_MISO          11
#define LORA_MOSI          10

// --- OLED PIN MAPPING ---
#define OLED_SDA           17
#define OLED_SCL           18
#define OLED_RST           21

// --- OLED OBJECT ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// --- RADIO OBJECT ---
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);

// --- INTERRUPT HANDLING ---
// Flag to indicate that a packet was received.
// Needs to be volatile to be safely accessed from an ISR.
volatile bool receivedFlag = false;

// This function is called when a complete packet is received by the module.
// It must be marked with ICACHE_RAM_ATTR for ESP32.
#if defined(ESP32)
IRAM_ATTR
#endif
void setFlag(void) {
  receivedFlag = true;
}
// --- GLOBAL STATE ---
const float IOHOME_FREQUENCIES[3] = { 868.25f, 868.95f, 869.85f };
uint8_t currentFreqIndex = 0;
float targetFreq = IOHOME_FREQUENCIES[0];

uint32_t lastHopTime = 0;
const uint32_t HOP_INTERVAL_MS = 10; // Hop every 10ms
const float RSSI_HOP_THRESHOLD = -95.0; // Pause hopping if signal is stronger than this

uint32_t lastRssiUpdate = 0;
uint32_t validPacketCount = 0;

// --- PACKET HISTORY ---
#define HISTORY_SIZE 3
char packetHistory[HISTORY_SIZE][32];
uint8_t historyCount = 0;

// --- IOHOME LIBRARY OBJECTS ---
// Define the channel based on targetFreq
IoHomeChannel_t ioHomeChannel = { .c0 = IOHOME_CHAN_C0, .c1 = IOHOME_CHAN_C1 };
// Placeholder NodeIDs and keys - replace with your actual values
NodeId sourceNodeId = {0x00, 0x00, 0x00};
NodeId destNodeId = {0x00, 0x00, 0x00};
uint8_t stackKey[16] = {0};
uint8_t systemKey[16] = {0};
IoHomeNode ioNode(&radio, &ioHomeChannel);

// --- PERSISTENT STATE MACHINE PARSER ---
class IoHomeStreamParser {
private:
    static const size_t BUFFER_SIZE = 1024;
    uint8_t buffer[BUFFER_SIZE];
    size_t length = 0;

    int frameBit = 0; // 0: wait for start, 1..8: data, 9: stop
    uint16_t currentData = 0;

    void consume(size_t count) {
        if (count >= length) {
            length = 0;
        } else {
            size_t remaining = length - count;
            memmove(buffer, buffer + count, remaining);
            length = remaining;
        }
    }

public:
    void pushBytes(const uint8_t* bytes, size_t len) {
        for (size_t i = 0; i < len; i++) {
            for (int b = 7; b >= 0; b--) {
                uint8_t bit = (bytes[i] >> b) & 0x01;
                if (frameBit == 0) {
                    if (bit == 0) { // Found Start bit (0)
                        frameBit = 1;
                        currentData = 0;
                    }
                } else if (frameBit >= 1 && frameBit <= 8) {
                    currentData |= (bit << (frameBit - 1)); // LSB first
                    frameBit++;
                } else if (frameBit == 9) {
                    if (bit == 1) { // Valid Stop bit (1)
                        if (length < BUFFER_SIZE) {
                            buffer[length++] = currentData;
                        }
                    }
                    frameBit = 0; // Reset for next byte
                }
            }
        }
    }

    bool extractNextFrame(IoHomeNode& ioNode, IoHomeFrame_t& outFrame, bool& outAuthStatus) {
        size_t searchIdx = 0;
        Serial.printf(">>> Searching rolling buffer (Current length: %zu bytes)\n", length);

        while (searchIdx + 2 + IOHOME_MIN_FRAME_LEN <= length) {
            if (buffer[searchIdx] == 0xFF && buffer[searchIdx+1] == 0x33) {
                size_t packetStart = searchIdx + 2;
                size_t remainingLen = length - packetStart;
                uint8_t* candidateData = &buffer[packetStart];

                size_t declaredBodyLen = (candidateData[0] & 0x1F) + 1;
                size_t expectedTotalLen = declaredBodyLen + IOHOME_FRAME_CRC_LEN;

                if (expectedTotalLen >= IOHOME_MIN_FRAME_LEN && expectedTotalLen <= 64) {
                    if (expectedTotalLen <= remainingLen) {
                        if (IoHomeNode::validateFrameCrc(candidateData, expectedTotalLen)) {
                            outAuthStatus = ioNode.parseFrame(candidateData, expectedTotalLen, outFrame);
                            consume(packetStart + expectedTotalLen); // Remove parsed packet
                            return true; // We found one!
                        }
                    } else {
                        Serial.printf("    [Parser] Found Sync Word, but packet is split. Waiting for next chunk.\n");
                        break; // Valid-looking header, but packet is split. Wait for next chunk.
                    }
                }
            }
            searchIdx++;
        }
        if (searchIdx > 0) consume(searchIdx); // Drop evaluated garbage
        if (length > BUFFER_SIZE - 256) consume(length - 64); // Safe fallback
        return false;
    }
};

IoHomeStreamParser streamParser;

void setup() {
  // 1. MANDATORY POWER SEQUENCE (The "Yellow Arrow" Fix)
  pinMode(HW_VEXT, OUTPUT);
  digitalWrite(HW_VEXT, LOW); // Pull LOW to power the SX1262

  pinMode(HW_LED, OUTPUT);
  digitalWrite(HW_LED, LOW);

  // Initialize OLED (Needs HW_VEXT to be LOW first, which we just did!)
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Heltec V3.2 IoHome");
  u8g2.drawStr(0, 30, "Booting...");
  u8g2.sendBuffer();

  // Initialize Serial and wait for connection
  Serial.begin(115200);
  while(!Serial);
  delay(1000);

  Serial.println(F("   HELTEC V3.2 IoHome NODE     "));
  Serial.println(F("==============================="));

  // 2. MANUAL RESET (Ensures Silicon is fresh)
  pinMode(LORA_NRST, OUTPUT);
  digitalWrite(LORA_NRST, LOW);
  delay(50);
  digitalWrite(LORA_NRST, HIGH);
  delay(200);

  // 3. SPI INITIALIZATION
  // NSS pin is managed by RadioLib, so it should not be passed to SPI.begin()
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI);

  // 4. RADIOLIB STARTUP
  Serial.print(F("Initializing Radio... "));
  // FSK Params: Freq, Bitrate, Dev, RX BW

  int state = radio.beginFSK(targetFreq, IOHOME_BITRATE, IOHOME_FREQ_DEV, IOHOME_RX_BW);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("SUCCESS"));

    // --- CRITICAL V3.2 TUNING ---
    radio.setDio2AsRfSwitch(true); // LINK PHYSICAL ANTENNA
    radio.setTCXO(1.6);            // V3.2 uses 32MHz TCXO
    radio.setRegulatorLDO();       // Quieter noise floor for RSSI
    radio.calibrateImage(targetFreq);

    // --- CONFIGURE IOHOME PACKET PARAMETERS ---
    Serial.print(F("Setting packet parameters... "));
    state = radio.setPreambleLength(IOHOME_PREAMBLE_LEN);
    if (state == RADIOLIB_ERR_NONE) {
      state = radio.setCRC(0); // Hardware CRC Off
    }
    if (state == RADIOLIB_ERR_NONE) {
      // Must use Fixed length because the length byte is PN9 whitened!
      state = radio.fixedPacketLengthMode(IOHOME_FIXED_PAYLOAD_LEN);
    }
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("SUCCESS"));
    } else {
      Serial.printf("FAILED, code %d\n", state);
    }

    // Set the actual sync word bytes
    Serial.print(F("Setting sync word... "));
    uint8_t syncWord[] = { (uint8_t)(IOHOME_HW_SYNC_WORD >> 16), (uint8_t)(IOHOME_HW_SYNC_WORD >> 8), (uint8_t)IOHOME_HW_SYNC_WORD };
    state = radio.setSyncWord(syncWord, IOHOME_HW_SYNC_WORD_LEN);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.printf("SUCCESS (0x%06X)\n", IOHOME_HW_SYNC_WORD);
    } else {
      Serial.printf("FAILED, code %d\n", state); // This is a critical failure
    }

    // Set the function to be called upon successful reception (interrupt)
    radio.setDio1Action(setFlag);

    // 5. START RECEIVING
    int startReceiveState = radio.startReceive();
    if (startReceiveState != RADIOLIB_ERR_NONE) {
      Serial.printf("Failed to start receive, code %d\n", startReceiveState);
      while (true); // Halt execution on critical error in setup
    }
    Serial.println(F("Radio is LISTENING."));

    // 6. INITIALIZE IOHOME LIBRARY
    Serial.print(F("Initializing IoHomeNode... "));
    state = ioNode.begin(&ioHomeChannel, sourceNodeId, destNodeId, stackKey, systemKey);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("SUCCESS"));
    } else {
        Serial.printf("FAILED (Error: %d)\n", state);
    }

    // Display Ready State
    u8g2.clearBuffer();
    u8g2.drawStr(0, 15, "Heltec V3.2 IoHome");
    u8g2.drawStr(0, 30, "Radio: LISTENING");
    u8g2.sendBuffer();

  } else {
    Serial.printf("FAILED (Error: %d)\n", state);
    u8g2.clearBuffer();
    u8g2.drawStr(0, 15, "RADIO INIT FAILED!");
    u8g2.sendBuffer();
    while(1);
  }
}

void loop() {
  // --- 0. CHANNEL HOPPING ---
  if (!receivedFlag && (millis() - lastHopTime >= HOP_INTERVAL_MS)) {
    float rssi = radio.getRSSI(false); // Check instantaneous RSSI

    if (rssi < RSSI_HOP_THRESHOLD) {
      // Channel is quiet (below noise floor), hop to the next frequency
      currentFreqIndex = (currentFreqIndex + 1) % 3;
      targetFreq = IOHOME_FREQUENCIES[currentFreqIndex];

      radio.standby();
      radio.setFrequency(targetFreq);
      radio.startReceive();
      lastHopTime = millis();
    } else {
      // Signal detected! Pause hopping for 20ms to allow the packet to arrive
      lastHopTime = millis() + 20;
    }
  }

  // --- 1. MONITOR RSSI (Ambient Noise) ---
  if (millis() - lastRssiUpdate > 1000) {
    lastRssiUpdate = millis();

    // Get instantaneous RSSI to verify the antenna is "live"
    float currentRssi = radio.getRSSI(false); // Use 'false' for instantaneous RSSI

    Serial.print(F("Freq: "));
    Serial.print(targetFreq);
    Serial.print(F(" MHz | "));
    Serial.print(F("RSSI: "));
    Serial.print(currentRssi);
    Serial.print(F(" dBm | "));

    if (currentRssi > -1.0) {
      Serial.println(F("STATUS: BLINDED (Hardware/Antenna Issue)"));
      digitalWrite(HW_LED, HIGH); // Constant LED means hardware fail
    } else {
      Serial.println(F("STATUS: OK"));
      // Short pulse to show life
      digitalWrite(HW_LED, HIGH); delay(10); digitalWrite(HW_LED, LOW);
    }
  }

  // --- 2. IOHOME PACKET HANDLING ---
  // Check if the interrupt flag has been set by the ISR
  if (receivedFlag) {
    // Reset the flag
    receivedFlag = false;

    // A packet was received, read it
    int len = radio.getPacketLength();
    byte byteArr[IOHOME_MAX_FIFO_LEN] = {0};
    int state = radio.readData(byteArr, len);

    if (state == RADIOLIB_ERR_NONE) {
      // Packet was read successfully
      Serial.print(F("[RAW] Received packet! RSSI: "));
      Serial.print(radio.getRSSI());
      Serial.print(F(" | Length: "));
      Serial.println(len);

      // Print the raw data as hex
      Serial.print(F("      Data: "));
      for(int i = 0; i < len; i++) {
        if(byteArr[i] < 0x10) {
          Serial.print(F("0"));
        }
        Serial.print(byteArr[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();

      // --- 1. CONTINUOUS STATE MACHINE DECODING ---
      // Feed raw bits into the state machine
      streamParser.pushBytes(byteArr, len);

      // --- 2. EXTRACT MULTIPLE/SPLIT FRAMES ---
      IoHomeFrame_t parsedFrame;
      bool isAuth;
      while (streamParser.extractNextFrame(ioNode, parsedFrame, isAuth)) {
          validPacketCount++;

          Serial.printf(">>> SUCCESSFULLY RECEIVED IOHOME FRAME #%lu (CRC OK) <<<\n", validPacketCount);
          Serial.printf("    Command: 0x%02X | Source: %02X%02X%02X | Dest: %02X%02X%02X\n",
                parsedFrame.commandId,
                parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2);

          if (!isAuth) {
              Serial.println(F("    >>> AES MAC VERIFICATION FAILED (Or No Keys) <<<"));
          } else {
              Serial.println(F("    >>> AES AUTHENTICATION SUCCESSFUL <<<"));
          }

          // Update OLED
          if (historyCount < HISTORY_SIZE) historyCount++;
          for (int h = HISTORY_SIZE - 1; h > 0; --h) {
              strncpy(packetHistory[h], packetHistory[h-1], 32);
          }
          snprintf(packetHistory[0], 32, "#%lu %02X%02X%02X>%02X%02X%02X %02X",
                   validPacketCount,
                   parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                   parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2,
                   parsedFrame.commandId);

          u8g2.clearBuffer();
          char headerBuf[32];
          snprintf(headerBuf, 32, "Valid Rx: %lu", validPacketCount);
          u8g2.drawStr(0, 12, headerBuf);
          for (int h = 0; h < historyCount; h++) {
              u8g2.drawStr(0, 28 + (h * 15), packetHistory[h]);
          }
          u8g2.sendBuffer();
      }

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // Packet was received, but is malformed
      Serial.println(F("[RAW] CRC error!"));

    } else {
      // Some other error occurred
      Serial.print(F("[RAW] Receive failed, code "));
      Serial.println(state);
    }

    // Put the radio back into listening mode
    radio.startReceive();
    lastHopTime = millis(); // Reset hopping timer
  }
}
