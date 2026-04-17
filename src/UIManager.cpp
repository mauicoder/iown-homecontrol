#include "UIManager.h"
#include "hal/BoardHAL.h"
#include <WiFi.h>
#include <WiFiProv.h>
#include <qrcode.h>

namespace UIManager {

// --- STATE ---
volatile bool _isProvisioning = false;

// --- PACKET HISTORY for OLED ---
#define HISTORY_SIZE 3
char packetHistory[HISTORY_SIZE][32];
uint8_t historyCount = 0;

// --- QR CODE RENDERING ---
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

// --- WIFI EVENT HANDLER ---
void sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            Serial.print("\n>>> Wi-Fi Connected! IP: ");
            Serial.println(WiFi.localIP());
            _isProvisioning = false;
            drawReadyScreen("IP: " + WiFi.localIP().toString());
            break;
        }
        case ARDUINO_EVENT_PROV_START: {
            _isProvisioning = true;
            Serial.println("\n>>> Wi-Fi Provisioning Started.");
            Serial.println(">>> Use the 'ESP BLE Prov' App. Device: PROV_IoHome, PoP: iown1234");
            Serial.println(">>> Or scan the QR code below directly from the app:\n");
            WiFiProv.printQR("PROV_IoHome", "iown1234", "ble");

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

// --- PUBLIC FUNCTIONS ---
void begin() {
    WiFi.onEvent(sysProvEvent);
    WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM, NETWORK_PROV_SECURITY_1, "iown1234", "PROV_IoHome");
}

void loop() {
    static uint32_t wifiTimeoutTimer = millis();
    if (!_isProvisioning && WiFi.status() != WL_CONNECTED) {
        if (millis() - wifiTimeoutTimer > 15000) {
            Serial.println("\n>>> Wi-Fi connection timed out! Wiping dead credentials and rebooting to Setup Mode... <<<");
            WiFi.disconnect(false, true);
            delay(500);
            ESP.restart();
        }
    } else {
        wifiTimeoutTimer = millis();
    }
}

void drawBootScreen() {
    BoardHAL::display.clearBuffer();
    BoardHAL::display.setFont(u8g2_font_6x10_tr);
    BoardHAL::display.drawStr(0, 15, BoardHAL::getBoardName());
    BoardHAL::display.drawStr(0, 30, "Booting...");
    BoardHAL::display.sendBuffer();
}

void drawReadyScreen(const String& status) {
    BoardHAL::display.clearBuffer();
    BoardHAL::display.setFont(u8g2_font_6x10_tr);
    BoardHAL::display.drawStr(0, 15, BoardHAL::getBoardName());
    BoardHAL::display.drawStr(0, 30, status.c_str());
    BoardHAL::display.drawStr(0, 45, "Radio: LISTENING");
    BoardHAL::display.sendBuffer();
}

void drawPacketHistory(uint32_t frameCount, const IoHomeFrame_t& frame) {
    if (historyCount < HISTORY_SIZE) historyCount++;
    for (int h = HISTORY_SIZE - 1; h > 0; --h) {
        strncpy(packetHistory[h], packetHistory[h-1], 32);
    }
    snprintf(packetHistory[0], 32, "#%lu %02X%02X%02X>%02X%02X%02X %02X",
             frameCount,
             frame.sourceMac.n0, frame.sourceMac.n1, frame.sourceMac.n2,
             frame.destMac.n0, frame.destMac.n1, frame.destMac.n2,
             frame.commandId);

    BoardHAL::display.clearBuffer();
    char headerBuf[32];
    snprintf(headerBuf, 32, "Valid Rx: %lu", frameCount);
    BoardHAL::display.drawStr(0, 12, headerBuf);
    for (int h = 0; h < historyCount; h++) {
        BoardHAL::display.drawStr(0, 28 + (h * 15), packetHistory[h]);
    }
    BoardHAL::display.sendBuffer();
}

bool isProvisioning() {
    return _isProvisioning;
}

} // namespace UIManager
