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
float targetFreq = IOHOME_FREQ;
uint32_t lastRssiUpdate = 0;

// --- IOHOME LIBRARY OBJECTS ---
// Define the channel based on targetFreq
IoHomeChannel_t ioHomeChannel = { .c0 = IOHOME_CHAN_C0, .c1 = IOHOME_CHAN_C1 };
// Placeholder NodeIDs and keys - replace with your actual values
NodeId sourceNodeId = {0x00, 0x00, 0x00};
NodeId destNodeId = {0x00, 0x00, 0x00};
uint8_t stackKey[16] = {0};
uint8_t systemKey[16] = {0};
IoHomeNode ioNode(&radio, &ioHomeChannel);

void setup() {
  // 1. MANDATORY POWER SEQUENCE (The "Yellow Arrow" Fix)
  pinMode(HW_VEXT, OUTPUT);
  digitalWrite(HW_VEXT, LOW); // Pull LOW to power the SX1262

  pinMode(HW_LED, OUTPUT);
  digitalWrite(HW_LED, LOW);

  // Initialize OLED (Needs HW_VEXT to be LOW first, which we just did!)
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
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
  // --- 1. MONITOR RSSI (Ambient Noise) ---
  if (millis() - lastRssiUpdate > 1000) {
    lastRssiUpdate = millis();

    // Get instantaneous RSSI to verify the antenna is "live"
    float currentRssi = radio.getRSSI(false); // Use 'false' for instantaneous RSSI

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

      // --- 1. EXTRACT 8-BIT DATA FROM 10-BIT UART FRAMES ---
      uint8_t decodedFrame[IOHOME_MAX_FIFO_LEN];
      size_t decodedLen = 0;
      uint16_t currentData = 0;
      int frameBit = 0; // 0: wait for start, 1..8: data, 9: stop
      int idleBits = 0;

      for(int i = 0; i < len; i++) {
        // The SX1262 packs bits MSB-first chronologically
        for(int b = 7; b >= 0; b--) {
          uint8_t bit = (byteArr[i] >> b) & 0x01;

          if (frameBit == 0) {
            if (bit == 0) {
              // Found Start bit (0)
              frameBit = 1;
              currentData = 0;
              idleBits = 0;
            } else {
              // Line is idle (1)
              idleBits++;
              if (idleBits > IOHOME_UART_IDLE_BITS_MAX && decodedLen > 0) {
                size_t expectedLen = (decodedFrame[0] & 0x1F) + 1 + 2;
                // Minimum valid frame size validation. If it's shorter, it's garbage noise.
                if (expectedLen >= IOHOME_MIN_FRAME_LEN && decodedLen >= expectedLen) {
                  goto decode_done;
                } else {
                  decodedLen = 0; // Reset and keep searching this buffer!
                }
              }
            }
          } else if (frameBit >= 1 && frameBit <= 8) {
            // Data bits (UART sends LSB first, so we shift into place)
            currentData |= (bit << (frameBit - 1));
            frameBit++;
          } else if (frameBit == 9) {
            // Validate Stop bit (1) to prevent framing errors
            if (bit == 1) {
              decodedFrame[decodedLen++] = currentData;
            } else {
              decodedLen = 0; // Framing error, discard chunk
            }
            frameBit = 0;
          }
        }
      }
      decode_done:

      // --- 1.5 TRUNCATE TRAILING NOISE/PADDING ---
      if (decodedLen > 0) {
        size_t expectedLen = (decodedFrame[0] & 0x1F) + 1 + 2; // FieldValue + 1 (Body) + 2 (CRC)
        if (expectedLen >= IOHOME_MIN_FRAME_LEN && expectedLen <= decodedLen) {
          decodedLen = expectedLen;
        } else {
          decodedLen = 0; // Completely invalid length
        }
      }

      // --- 2. DE-WHITEN THE PAYLOAD ---
      // 1-way remotes use Direct Mode (UART bit-banging) so the payload is NOT PN9 whitened over the air!

      if (decodedLen > 0) {
        Serial.print(F("[IOHOME] Decoded: "));
        for(size_t i = 0; i < decodedLen; i++) {
          if(decodedFrame[i] < 0x10) Serial.print(F("0"));
          Serial.print(decodedFrame[i], HEX);
          Serial.print(F(" "));
        }
        Serial.println();

        // --- 3. ATTEMPT TO PARSE ---
        IoHomeFrame_t parsedFrame;
        if (ioNode.parseFrame(decodedFrame, decodedLen, parsedFrame)) {
            Serial.println(F(">>> SUCCESSFULLY PARSED IOHOME FRAME <<<"));
            Serial.printf("Command: 0x%02X | Source: %02X%02X%02X | Dest: %02X%02X%02X\n",
                  parsedFrame.commandId,
                  parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                  parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2);

            // Update OLED Display
            u8g2.clearBuffer();
            char buf[32];
            sprintf(buf, "Cmd: 0x%02X", parsedFrame.commandId);
            u8g2.drawStr(0, 15, buf);
            sprintf(buf, "Src: %02X%02X%02X", parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2);
            u8g2.drawStr(0, 35, buf);
            sprintf(buf, "Dst: %02X%02X%02X", parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2);
            u8g2.drawStr(0, 55, buf);
            u8g2.sendBuffer();
        } else {
            Serial.println(F(">>> PARSE FAILED (Expected until AES keys are provided) <<<"));
        }
      } else {
        Serial.println(F("[RAW] Discarded noise/ghost packet during UART extraction."));
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
  }
}
