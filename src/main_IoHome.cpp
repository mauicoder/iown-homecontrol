#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "IoHome.h" // Include the IoHome library header
#include <Wire.h>
#include <mbedtls/aes.h>
#include <Preferences.h>
#include <U8g2lib.h>
#include "IoHomeParser.h"
#include "IoHomeCrypto.h"
#include "IoHomeWebSniffer.h"
#include "hal/BoardHAL.h" // Include the hardware abstraction layer for board-specific configurations

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

Preferences preferences;

// Default to zeroed keys/addresses. Will be loaded from NVRAM on boot.
NodeId sourceNodeId = {0x00, 0x00, 0x00};
NodeId destNodeId = {0x00, 0x00, 0x00};
uint8_t stackKey[16] = {0};
uint8_t systemKey[16] = {0};
IoHomeNode ioNode(BoardHAL::radio, &ioHomeChannel);
IoHomeWebSniffer webSniffer;

// --- CONFIGURATION MANAGEMENT ---
void loadConfiguration() {
    preferences.begin("iohome", true); // true = read-only mode

    if (preferences.getBytesLength("sourceNode") == sizeof(NodeId)) {
        preferences.getBytes("sourceNode", &sourceNodeId, sizeof(NodeId));
        Serial.printf("Loaded NodeID: %02X%02X%02X\n", sourceNodeId.n0, sourceNodeId.n1, sourceNodeId.n2);
    } else {
        Serial.println("No NodeID in config. Using defaults.");
    }

    if (preferences.getBytesLength("destNode") == sizeof(NodeId)) {
        preferences.getBytes("destNode", &destNodeId, sizeof(NodeId));
    }

    if (preferences.getBytesLength("stackKey") == 16) {
        preferences.getBytes("stackKey", stackKey, 16);
        Serial.println("Loaded 16-byte Stack Key from config.");
    } else {
        Serial.println("No Stack Key in config. Using defaults.");
    }

    if (preferences.getBytesLength("seqCounter") == sizeof(uint16_t)) {
        uint16_t sc = 0;
        preferences.getBytes("seqCounter", &sc, sizeof(uint16_t));
        ioNode.setSequenceCounter(sc);
        Serial.printf("Loaded Sequence Counter: 0x%04X\n", sc);
    }

    preferences.end();
}

void saveConfiguration(NodeId srcNode, NodeId dstNode, const uint8_t* newKey) {
    preferences.begin("iohome", false); // false = read/write mode
    preferences.putBytes("sourceNode", &srcNode, sizeof(NodeId));
    preferences.putBytes("destNode", &dstNode, sizeof(NodeId));
    preferences.putBytes("stackKey", newKey, 16);
    preferences.end();

    // Update active RAM variables
    sourceNodeId = srcNode;
    destNodeId = dstNode;
    memcpy(stackKey, newKey, 16);
    Serial.println("Configuration successfully saved to NVRAM!");
}

void saveSequenceCounter(uint16_t counter) {
    preferences.begin("iohome", false);
    preferences.putBytes("seqCounter", &counter, sizeof(uint16_t));
    preferences.end();
    Serial.printf("    >>> SEQUENCE COUNTER SAVED: 0x%04X <<<\n", counter);
}

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

    void clear() {
        length = 0;
        frameBit = 0;
        currentData = 0;
    }
};

IoHomeStreamParser streamParser;

void setup() {
  // 1. HARDWARE INIT
  BoardHAL::initPower();

  // 2. DISPLAY INIT
  BoardHAL::display.begin();
  BoardHAL::display.setFont(u8g2_font_6x10_tr);
  BoardHAL::display.clearBuffer();
  BoardHAL::display.drawStr(0, 15, BoardHAL::getBoardName());
  BoardHAL::display.drawStr(0, 30, "Booting...");
  BoardHAL::display.sendBuffer();

  // Initialize Serial and wait for connection
  Serial.begin(115200);
  while(!Serial);
  delay(1000);

  Serial.println(F("   HELTEC V3.2 IoHome NODE     "));
  Serial.println(F("==============================="));

  loadConfiguration();
  webSniffer.begin();

  // 3. RADIOLIB STARTUP
  Serial.print(F("Initializing Radio... "));
  uint8_t syncWord[] = { (uint8_t)(IOHOME_HW_SYNC_WORD >> 16), (uint8_t)(IOHOME_HW_SYNC_WORD >> 8), (uint8_t)IOHOME_HW_SYNC_WORD };
  bool radioSuccess = BoardHAL::initRadioProtocol(targetFreq, IOHOME_BITRATE, IOHOME_FREQ_DEV, IOHOME_RX_BW, IOHOME_PREAMBLE_LEN, IOHOME_FIXED_PAYLOAD_LEN, syncWord, IOHOME_HW_SYNC_WORD_LEN, setFlag);

  if (radioSuccess) {
    Serial.println(F("SUCCESS"));

    // 4. START RECEIVING
    int startReceiveState = BoardHAL::startReceive();
    if (startReceiveState != RADIOLIB_ERR_NONE) {
      Serial.printf("Failed to start receive, code %d\n", startReceiveState);
      while (true); // Halt execution on critical error in setup
    }
    Serial.println(F("Radio is LISTENING."));

    // 6. INITIALIZE IOHOME LIBRARY
    Serial.print(F("Initializing IoHomeNode... "));
    int state = ioNode.begin(&ioHomeChannel, sourceNodeId, destNodeId, stackKey, systemKey);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("SUCCESS"));
    } else {
        Serial.printf("FAILED (Error: %d)\n", state);
    }

    // Display Ready State
    BoardHAL::display.clearBuffer();
    BoardHAL::display.drawStr(0, 15, BoardHAL::getBoardName());
    BoardHAL::display.drawStr(0, 30, "Radio: LISTENING");
    BoardHAL::display.sendBuffer();

  } else {
    Serial.println(F("RADIO INIT FAILED"));
    BoardHAL::display.clearBuffer();
    BoardHAL::display.drawStr(0, 15, "RADIO INIT FAILED!");
    BoardHAL::display.sendBuffer();
    while(1);
  }
}

void loop() {
  // Handle Web Server Clients
  webSniffer.loop();

  // --- 0. CHANNEL HOPPING ---
  if (!receivedFlag && (millis() - lastHopTime >= HOP_INTERVAL_MS)) {
    float rssi = BoardHAL::getInstantaneousRSSI();

    if (rssi < RSSI_HOP_THRESHOLD) {
      // Channel is quiet (below noise floor), hop to the next frequency
      currentFreqIndex = (currentFreqIndex + 1) % 3;
      targetFreq = IOHOME_FREQUENCIES[currentFreqIndex];

      BoardHAL::radio->standby();
      BoardHAL::radio->setFrequency(targetFreq);
      BoardHAL::startReceive();
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
    float currentRssi = BoardHAL::getInstantaneousRSSI();

    Serial.print(F("Freq: "));
    Serial.print(targetFreq);
    Serial.print(F(" MHz | "));
    Serial.print(F("RSSI: "));
    Serial.print(currentRssi);
    Serial.print(F(" dBm | "));

    if (currentRssi > -1.0) {
      Serial.println(F("STATUS: BLINDED (Hardware/Antenna Issue)"));
      BoardHAL::setLed(true); // Constant LED means hardware fail
    } else {
      Serial.println(F("STATUS: OK"));
      // Short pulse to show life
      BoardHAL::setLed(true); delay(10); BoardHAL::setLed(false);
    }
  }

  // --- 2. IOHOME PACKET HANDLING ---

  // Check if the interrupt flag has been set by the ISR
  if (receivedFlag) {
    // Reset the flag
    receivedFlag = false;

    // A packet was received, read it
    size_t len = 0;
    byte byteArr[IOHOME_MAX_FIFO_LEN] = {0};
    int state = BoardHAL::readData(byteArr, len);

    if (state == RADIOLIB_ERR_NONE) {
      // Packet was read successfully
      Serial.print(F("[RAW] Received packet! RSSI: "));
      Serial.print(BoardHAL::radio->getRSSI());
      Serial.print(F(" | Length: "));
      Serial.println((int)len);

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

      // Log raw bytes to the web sniffer
      webSniffer.appendRawPacket(targetFreq, BoardHAL::radio->getRSSI(), len, byteArr);

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

          webSniffer.appendDecodedPacket(validPacketCount, parsedFrame, isAuth);

          // Automatically save 1-Way Key Transfer to NVRAM
          if (parsedFrame.commandId == 0x30 && parsedFrame.payload.size() >= 16) {
              uint8_t extracted_key[16];
              IoHomeCrypto::decryptTransferKey(parsedFrame, extracted_key);

              Serial.println(F("    >>> NEW 1-WAY KEY DETECTED! SAVING TO NVRAM <<<"));
              Serial.print(F("    Extracted Key: "));
              for (int i = 0; i < 16; i++) {
                  Serial.printf("%02X ", extracted_key[i]);
              }
              Serial.println();

              saveConfiguration(parsedFrame.sourceMac, destNodeId, extracted_key);

              // Reload the IoHomeNode with the new keys immediately
              ioNode.begin(&ioHomeChannel, sourceNodeId, destNodeId, stackKey, systemKey);
          }

          // Decode 0x00 Execute Function payloads to see the button presses
          if (parsedFrame.commandId == 0x00 && isAuth && parsedFrame.payload.size() >= 4) {
              uint16_t action = (parsedFrame.payload[2] << 8) | parsedFrame.payload[3];
              Serial.print("    >>> DECODED ACTION: ");
              if (action == IOHOME_ACTION_UP) Serial.print("UP / OPEN");
              else if (action == IOHOME_ACTION_DOWN) Serial.print("DOWN / CLOSE");
              else if (action == IOHOME_ACTION_MY) Serial.print("MY / STOP");
              else Serial.printf("UNKNOWN (0x%04X)", action);

              if (parsedFrame.payload.size() >= 6) {
                  uint16_t param = (parsedFrame.payload[4] << 8) | parsedFrame.payload[5];
                  Serial.printf(" | PARAM: 0x%04X <<<\n", param);
              } else {
                  Serial.println(" <<<");
              }
          }

          // Automatically learn the targeted Awning's address from authenticated 0x00 commands
          if (parsedFrame.commandId == 0x00 && isAuth && parsedFrame.destMac.n2 != 0x3F) {
              if (parsedFrame.destMac.n0 != destNodeId.n0 || parsedFrame.destMac.n1 != destNodeId.n1 || parsedFrame.destMac.n2 != destNodeId.n2) {
                  Serial.printf("    >>> TARGET DEVICE DETECTED (%02X%02X%02X)! SAVING TO NVRAM <<<\n",
                                parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2);
                  saveConfiguration(sourceNodeId, parsedFrame.destMac, stackKey);
                  ioNode.begin(&ioHomeChannel, sourceNodeId, destNodeId, stackKey, systemKey);
              }
          }

          // Automatically sync the Sequence Counter to NVRAM so we don't lose it on reboot
          if (isAuth) {
              saveSequenceCounter(ioNode.getSequenceCounter());
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

          BoardHAL::display.clearBuffer();
          char headerBuf[32];
          snprintf(headerBuf, 32, "Valid Rx: %lu", validPacketCount);
          BoardHAL::display.drawStr(0, 12, headerBuf);
          for (int h = 0; h < historyCount; h++) {
              BoardHAL::display.drawStr(0, 28 + (h * 15), packetHistory[h]);
          }
          BoardHAL::display.sendBuffer();
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
    BoardHAL::radio->standby(); // Flush SX1276 FIFO to clear stuck DIO0 interrupts
    BoardHAL::startReceive();
    lastHopTime = millis(); // Reset hopping timer
  }

  // --- 3. SERIAL INTERACTION (User Commands) ---
  if (Serial.available() > 0) {
      char c = Serial.read();
      // Flush trailing newlines
      while(Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r')) Serial.read();

      if (c == 'U' || c == 'u' || c == 'D' || c == 'd' || c == 'S' || c == 's' || c == 'm' || c == 'M') {
          BoardHAL::radio->standby(); // Pause receiving to free up the radio chip
          int16_t state = RADIOLIB_ERR_NONE;

          if (c == 'U' || c == 'u') {
              Serial.println("\n>>> USER COMMAND: SENDING 'UP' <<<");
              state = ioNode.sendButton(IOHOME_ACTION_UP);
          } else if (c == 'D' || c == 'd') {
              Serial.println("\n>>> USER COMMAND: SENDING 'DOWN' <<<");
              state = ioNode.sendButton(IOHOME_ACTION_DOWN);
          } else {
              Serial.println("\n>>> USER COMMAND: SENDING 'MY/STOP' <<<");
              state = ioNode.sendButton(IOHOME_ACTION_MY);
          }

          if (state == RADIOLIB_ERR_NONE) Serial.println("    >>> TRANSMISSION SUCCESSFUL <<<");
          else Serial.printf("    >>> TRANSMISSION FAILED (Code: %d) <<<\n", state);

          // Save the incremented counter after we transmit
          saveSequenceCounter(ioNode.getSequenceCounter());

          BoardHAL::startReceive(); // Resume listening mode

          // --- CRITICAL FIX FOR THE "GHOST PACKET" ---
          // The radio.transmit() function triggers the DIO1 interrupt when TxDone occurs.
          // This sets receivedFlag to true. We MUST clear it here, otherwise the main loop will
          // immediately read the dirty SX1262 FIFO and repeatedly parse the old 0x0EF4 packet.
          receivedFlag = false;

          // Flush the software buffer to completely eliminate ghost packets
          streamParser.clear();

          lastHopTime = millis();
      }
  }
}
