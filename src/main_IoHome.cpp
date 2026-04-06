#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "IoHome.h" // Include the IoHome library header

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
float targetFreq = 868.95;
uint32_t lastRssiUpdate = 0;

// --- IOHOME LIBRARY OBJECTS ---
// Define the channel based on targetFreq (868.95 MHz -> c0=868, c1=95)
IoHomeChannel_t ioHomeChannel = { .c0 = 868, .c1 = 95 };
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
  // FSK Params: Freq, Bitrate (38.4), Dev (19.2), RX BW (156.2)

  int state = radio.beginFSK(targetFreq, 38.4, 19.2, 156.2);

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
      // 64 bytes is enough to safely capture the entire io-homecontrol packet + some trailing noise
      state = radio.fixedPacketLengthMode(64);
    }
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("SUCCESS"));
    } else {
      Serial.printf("FAILED, code %d\n", state);
    }

    // Set the actual sync word bytes
    Serial.print(F("Setting sync word... "));
    // io-homecontrol original hardware uses Direct Mode (bypassing the hardware Sync Word entirely).
    // The true physical Sync Word is the UART-encoded bits of "FF 33", which precisely evaluate to 0x57FD99!
    uint8_t syncWord[] = { 0x57, 0xFD, 0x99 };
    state = radio.setSyncWord(syncWord, 3);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("SUCCESS (0x57FD99)"));
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
  } else {
    Serial.printf("FAILED (Error: %d)\n", state);
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
    // Allocate 256 bytes to safely handle the maximum possible SX1262 FIFO size
    int len = radio.getPacketLength();
    byte byteArr[256] = {0};
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
      uint8_t decodedFrame[256];
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
              // In 8-N-1 UART, >15 consecutive 1s guarantees the transmission is finished
              if (idleBits > 15 && decodedLen > 0) {
                goto decode_done;
              }
            }
          } else if (frameBit >= 1 && frameBit <= 8) {
            // Data bits (UART sends LSB first, so we shift into place)
            currentData |= (bit << (frameBit - 1));
            frameBit++;
          } else if (frameBit == 9) {
            // Stop bit (1)
            decodedFrame[decodedLen++] = currentData;
            frameBit = 0;
          }
        }
      }
      decode_done:

      // --- 1.5 TRUNCATE TRAILING NOISE/PADDING ---
      if (decodedLen > 0) {
        size_t expectedLen = (decodedFrame[0] & 0x1F) + 1 + 2; // FieldValue + 1 (Body) + 2 (CRC)
        if (expectedLen <= decodedLen) {
          decodedLen = expectedLen;
        }
      }

      // --- 2. DE-WHITEN THE PAYLOAD ---
      // 1-way remotes use Direct Mode (UART bit-banging) so the payload is NOT PN9 whitened over the air!

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
      } else {
          Serial.println(F(">>> PARSE FAILED (Expected until AES keys are provided) <<<"));
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
