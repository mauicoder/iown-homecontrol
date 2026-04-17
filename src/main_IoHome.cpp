#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "IoHome.h" // Include the IoHome library header
#include "MqttManager.h" // Include the MQTT Manager header
#include <Wire.h>
#include <mbedtls/aes.h>
#include <U8g2lib.h>
#include "IoHomeParser.h"
#include "IoHomeCrypto.h"
#include "IoHomeWebSniffer.h"
#include "ConfigManager.h"
#include "IoHomeStreamParser.h"
#include "hal/BoardHAL.h" // Include the hardware abstraction layer for board-specific configurations
#include <WiFiProv.h>
#include <qrcode.h>

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

// --- PACKET HISTORY ---
#define HISTORY_SIZE 3
char packetHistory[HISTORY_SIZE][32];
uint8_t historyCount = 0;

// --- IOHOME LIBRARY OBJECTS ---
// Define the channel based on targetFreq
IoHomeChannel_t ioHomeChannel = { .c0 = IOHOME_CHAN_C0, .c1 = IOHOME_CHAN_C1 };

uint32_t validPacketCount = 0;
uint32_t lastPacketTime = 0;

IoHomeNode ioNode(BoardHAL::radio, &ioHomeChannel);
IoHomeWebSniffer webSniffer;

void updateDisplay(); // Forward declaration

// --- Loop Logic Functions ---
void handleMqttAndWeb();
void handleDisplayUpdates();
void handleWifiHealing();
void handleChannelHopping();
void handleReceivedPacket();
void handleUserCommands();

volatile bool isProvisioning = false;

// --- DISPLAY UPDATE ---
void updateDisplay() {
    if (isProvisioning) return;

    BoardHAL::display.clearBuffer();
    BoardHAL::display.setFont(u8g2_font_6x10_tr);

    bool showHistory = (validPacketCount > 0) && (millis() - lastPacketTime <= 10000);

    if (!showHistory) {
        BoardHAL::display.drawStr(0, 15, BoardHAL::getBoardName());
        if (WiFi.status() == WL_CONNECTED) {
            String ipStr = "IP: " + WiFi.localIP().toString();
            BoardHAL::display.drawStr(0, 30, ipStr.c_str());
        } else {
            BoardHAL::display.drawStr(0, 30, "Waiting for WiFi...");
        }

        char radioBuf[32];
        snprintf(radioBuf, sizeof(radioBuf), "Radio: Rx %lu", validPacketCount);
        BoardHAL::display.drawStr(0, 45, radioBuf);

        String mqttStr = "MQTT: ";
        if (strlen(ConfigManager::mqttConfig.server) == 0) mqttStr += "Unconfigured";
        else if (MqttManager::isConnected()) mqttStr += "Connected";
        else mqttStr += "Disconnected";
        BoardHAL::display.drawStr(0, 60, mqttStr.c_str());
    } else {
        char headerBuf[32];
        const char* mState = (strlen(ConfigManager::mqttConfig.server) == 0) ? "No Cfg" : (MqttManager::isConnected() ? "OK" : "ERR");
        snprintf(headerBuf, 32, "Rx:%lu | MQTT:%s", validPacketCount, mState);
        BoardHAL::display.drawStr(0, 12, headerBuf);
        for (int h = 0; h < historyCount; h++) {
            BoardHAL::display.drawStr(0, 28 + (h * 15), packetHistory[h]);
        }
    }
    BoardHAL::display.sendBuffer();
}

static void renderOledQR(esp_qrcode_handle_t qrcode) {
    int size = esp_qrcode_get_size(qrcode);
    int scale = (size > 30) ? 1 : 2; // Auto-scale to ensure it fits the 64px height
    int x0 = (128 - (size * scale)) / 2;
    int y0 = (64 - (size * scale)) / 2;

    if (y0 < 0) y0 = 0; // Safety clamp

    BoardHAL::display.clearBuffer();

    // Draw a white background square for the QR code to ensure contrast
    BoardHAL::display.setDrawColor(1);
    BoardHAL::display.drawBox(x0 - 2, y0 - 2, (size * scale) + 4, (size * scale) + 4);

    // Draw the black QR modules
    BoardHAL::display.setDrawColor(0);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (esp_qrcode_get_module(qrcode, x, y)) {
                BoardHAL::display.drawBox(x0 + (x * scale), y0 + (y * scale), scale, scale);
            }
        }
    }
    BoardHAL::display.setDrawColor(1); // Restore white text color
    BoardHAL::display.setFont(u8g2_font_5x7_tr);
    BoardHAL::display.drawStr(0, 20, "Scan");
    BoardHAL::display.drawStr(0, 30, "App");
    BoardHAL::display.drawStr(0, 40, "QR");
    BoardHAL::display.sendBuffer();
}

void sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            Serial.print("\n>>> Wi-Fi Connected! IP: ");
            Serial.println(WiFi.localIP());
            isProvisioning = false;
            updateDisplay();
            break;
        }
        case ARDUINO_EVENT_PROV_START: {
            isProvisioning = true;
            Serial.println("\n>>> Wi-Fi Provisioning Started.");
            Serial.println(">>> Use the 'ESP BLE Prov' App. Device: PROV_IoHome, PoP: iown1234");
            Serial.println(">>> Or scan the QR code below directly from the app:\n");
            WiFiProv.printQR("PROV_IoHome", "iown1234", "ble");

            // Render QR to OLED using ESP32's built-in QR library
            const char* qrPayload = "{\"ver\":\"v1\",\"name\":\"PROV_IoHome\",\"pop\":\"iown1234\",\"transport\":\"ble\"}";
            esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
            cfg.display_func = renderOledQR;
            esp_qrcode_generate(&cfg, qrPayload);
            break;
        }
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("\n>>> Provisioning Successful!");
            break;
        default:
            break;
    }
}

IoHomeStreamParser streamParser;

extern volatile char webCommandTarget; // Hook into the Web Sniffer commands
extern volatile uint8_t webCommandDevice;

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

  ConfigManager::load(ioNode);
  MqttManager::begin(ConfigManager::mqttConfig);

  // Setup WiFi Provisioning
  WiFi.onEvent(sysProvEvent);
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM, NETWORK_PROV_SECURITY_1, "iown1234", "PROV_IoHome");
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

    Serial.println(F("IoHomeNode initialized with multi-profile support."));

    // Display Ready State
    updateDisplay();

  } else {
    Serial.println(F("RADIO INIT FAILED"));
    BoardHAL::display.clearBuffer();
    BoardHAL::display.drawStr(0, 15, "RADIO INIT FAILED!");
    BoardHAL::display.sendBuffer();
    while(1);
  }
}

void handleMqttAndWeb() {
    MqttManager::loop();
    webSniffer.loop();
}

void handleDisplayUpdates() {
    static bool lastMqttState = false;
    bool currentMqttState = MqttManager::isConnected();
    bool mqttChanged = (currentMqttState != lastMqttState);
    if (mqttChanged) {
        lastMqttState = currentMqttState;
    }

    static bool showingHistory = false;
    bool shouldShowHistory = (validPacketCount > 0) && (millis() - lastPacketTime <= 10000);
    bool historyChanged = (showingHistory != shouldShowHistory);
    if (historyChanged) {
        showingHistory = shouldShowHistory;
    }

    if (mqttChanged || historyChanged) {
        updateDisplay();
    }
}

void handleWifiHealing() {
    static uint32_t wifiTimeoutTimer = millis();
    if (!isProvisioning && WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiTimeoutTimer > 15000) {
            Serial.println("\n>>> Wi-Fi connection timed out! Wiping dead credentials and rebooting to Setup Mode... <<<");
            WiFi.disconnect(false, true);
            delay(500);
            ESP.restart();
        }
    } else {
        wifiTimeoutTimer = millis(); // Reset timer while connected or provisioning
    }
}

void handleChannelHopping() {
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
}

void handleUserCommands() {
    if (Serial.available() > 0 || webCommandTarget != 0) {
      char c = 0;
      uint8_t targetDevice = 0; // Default to Slot 1 for serial input
      bool execute = false;

      if (webCommandTarget != 0) {
          c = webCommandTarget;
          targetDevice = webCommandDevice;
          webCommandTarget = 0; // Clear the flag after reading
          execute = true;
      } else {
          static String serialBuf = "";
          while (Serial.available() > 0) {
              char inChar = Serial.read();
              if (inChar == '\n' || inChar == '\r') {
                  if (serialBuf.length() > 0) {
                      c = serialBuf[0];
                      if (serialBuf.length() > 1 && serialBuf[1] >= '1' && serialBuf[1] <= '5') {
                          targetDevice = serialBuf[1] - '1';
                      }
                      serialBuf = ""; // Clear buffer
                      execute = true;
                  }
              } else {
                  serialBuf += inChar;
                  // Auto-execute if 2 characters are typed (e.g. "U2") without pressing Enter
                  if (serialBuf.length() == 2) {
                      c = serialBuf[0];
                      if (serialBuf[1] >= '1' && serialBuf[1] <= '5') {
                          targetDevice = serialBuf[1] - '1';
                      }
                      serialBuf = ""; // Clear buffer
                      execute = true;
                  }
              }
          }
      }

      if (execute && (c == 'U' || c == 'u' || c == 'D' || c == 'd' || c == 'S' || c == 's' || c == 'm' || c == 'M')) {
          BoardHAL::radio->standby(); // Pause receiving to free up the radio chip
          int16_t state = RADIOLIB_ERR_NONE;

          if (c == 'U' || c == 'u') {
              Serial.printf("\n>>> USER COMMAND: SENDING 'UP' ON CH %d <<<\n", targetDevice + 1);
              state = ioNode.sendButton(IOHOME_ACTION_UP, targetDevice);
          } else if (c == 'D' || c == 'd') {
              Serial.printf("\n>>> USER COMMAND: SENDING 'DOWN' ON CH %d <<<\n", targetDevice + 1);
              state = ioNode.sendButton(IOHOME_ACTION_DOWN, targetDevice);
          } else {
              Serial.printf("\n>>> USER COMMAND: SENDING 'MY/STOP' ON CH %d <<<\n", targetDevice + 1);
              state = ioNode.sendButton(IOHOME_ACTION_MY, targetDevice);
          }

          if (state == RADIOLIB_ERR_NONE) Serial.println("    >>> TRANSMISSION SUCCESSFUL <<<");
          else Serial.printf("    >>> TRANSMISSION FAILED (Code: %d) <<<\n", state);

          // Save the incremented counter for the active profile
          ConfigManager::saveDevices(ioNode);

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

void handleReceivedPacket() {
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
        for(size_t i = 0; i < len; i++) {
            if(byteArr[i] < 0x10) Serial.print(F("0"));
            Serial.print(byteArr[i], HEX);
            Serial.print(F(" "));
        }
        Serial.println();

        webSniffer.appendRawPacket(targetFreq, BoardHAL::radio->getRSSI(), len, byteArr);
        streamParser.pushBytes(byteArr, len);

        IoHomeFrame_t parsedFrame;
        bool isAuth;
        while (streamParser.extractNextFrame(ioNode, parsedFrame, isAuth)) {
            validPacketCount++;
            lastPacketTime = millis();

            Serial.printf(">>> SUCCESSFULLY RECEIVED IOHOME FRAME #%lu (CRC OK) <<<\n", validPacketCount);
            Serial.printf("    Command: 0x%02X | Source: %02X%02X%02X | Dest: %02X%02X%02X\n",
                          parsedFrame.commandId,
                          parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                          parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2);

            if (!isAuth) Serial.println(F("    >>> AES MAC VERIFICATION FAILED (Or No Keys) <<<"));
            else Serial.println(F("    >>> AES AUTHENTICATION SUCCESSFUL <<<"));

            webSniffer.appendDecodedPacket(validPacketCount, parsedFrame, isAuth);

            // Automatically save 1-Way Key Transfer to NVRAM
            if (parsedFrame.commandId == 0x30 && parsedFrame.payload.size() >= 16) {
                uint8_t extracted_key[16];
                IoHomeCrypto::decryptTransferKey(parsedFrame, extracted_key);

                Serial.println(F("    >>> NEW 1-WAY KEY DETECTED! SAVING TO NVRAM <<<"));
                int slot = -1;
                for (int i=0; i<5; i++) { if (ConfigManager::devices[i].active && memcmp(&ConfigManager::devices[i].sourceMac, &parsedFrame.sourceMac, 3) == 0) { slot = i; break; } }
                if (slot == -1) { for (int i=0; i<5; i++) { if (!ConfigManager::devices[i].active) { slot = i; break; } } }

                if (slot != -1) {
                    ConfigManager::devices[slot].active = true;
                    ConfigManager::devices[slot].sourceMac = parsedFrame.sourceMac;
                    memcpy(ConfigManager::devices[slot].stackKey, extracted_key, 16);
                    ioNode.loadProfiles(ConfigManager::devices, 5);
                    ConfigManager::saveDevices(ioNode);
                    Serial.printf("    >>> SAVED KEY TO CHANNEL SLOT %d <<<\n", slot + 1);
                } else {
                    Serial.println("    >>> NO EMPTY PROFILE SLOTS AVAILABLE! <<<");
                }
            }

            // Decode 0x00 Execute Function payloads
            if (parsedFrame.commandId == 0x00 && isAuth && parsedFrame.payload.size() >= 4) {
                uint16_t action = (parsedFrame.payload[2] << 8) | parsedFrame.payload[3];
                Serial.print("    >>> DECODED ACTION: ");
                if (action == IOHOME_ACTION_UP) Serial.print("UP / OPEN");
                else if (action == IOHOME_ACTION_DOWN) Serial.print("DOWN / CLOSE");
                else if (action == IOHOME_ACTION_MY) Serial.print("MY / STOP");
                else Serial.printf("UNKNOWN (0x%04X)", action);
                Serial.println(" <<<");
            }

            // Automatically learn the targeted Awning's address
            if (parsedFrame.commandId == 0x00 && isAuth && parsedFrame.destMac.n2 != 0x3F) {
                for (int i=0; i<5; i++) {
                    if (ConfigManager::devices[i].active && memcmp(&ConfigManager::devices[i].sourceMac, &parsedFrame.sourceMac, 3) == 0) {
                        if (memcmp(&ConfigManager::devices[i].destMac, &parsedFrame.destMac, 3) != 0) {
                            Serial.printf("    >>> TARGET DEVICE DETECTED (%02X%02X%02X) ON CH %d! SAVING <<<\n",
                                          parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2, i + 1);
                            ioNode.getProfiles()[i].destMac = parsedFrame.destMac;
                            ConfigManager::saveDevices(ioNode);
                        }
                        break;
                    }
                }
            }

            if (isAuth) ConfigManager::saveDevices(ioNode); // Save seq counter increments

            // Update OLED
            if (!isProvisioning) {
                if (historyCount < HISTORY_SIZE) historyCount++;
                for (int h = HISTORY_SIZE - 1; h > 0; --h) strncpy(packetHistory[h], packetHistory[h-1], 32);
                snprintf(packetHistory[0], 32, "#%lu %02X%02X%02X>%02X%02X%02X %02X",
                         validPacketCount,
                         parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                         parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2,
                         parsedFrame.commandId);
                updateDisplay();
            }
        }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        Serial.println(F("[RAW] CRC error!"));
    } else {
        Serial.printf("[RAW] Receive failed, code %d\n", state);
    }

    // Put the radio back into listening mode
    BoardHAL::radio->standby(); // Flush SX1276 FIFO to clear stuck DIO0 interrupts
    BoardHAL::startReceive();
    lastHopTime = millis(); // Reset hopping timer
}

void loop() {
    handleMqttAndWeb();
    handleDisplayUpdates();
    handleWifiHealing();
    handleChannelHopping();

    if (receivedFlag) {
        handleReceivedPacket();
    }

    handleUserCommands();
}
