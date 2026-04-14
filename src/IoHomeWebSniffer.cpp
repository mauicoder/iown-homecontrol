#include "IoHomeWebSniffer.h"
#include "IoHome.h"
#include "hal/BoardHAL.h"
#include <qrcode.h>

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

volatile char webCommandTarget = 0; // Global variable to pass web commands to the main loop
volatile uint8_t webCommandDevice = 0;
volatile bool isProvisioning = false;

extern IoHomeProfile devices[5]; // Access the globally enrolled devices
extern IoHomeNode ioNode; // Needed to update the active profile in RAM
extern void saveConfiguration(); // Needed to write changes to NVRAM

IoHomeWebSniffer::IoHomeWebSniffer() : _server(80) {}

void IoHomeWebSniffer::sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
            Serial.print("\n>>> Wi-Fi Connected! IP: ");
            Serial.println(WiFi.localIP());
            isProvisioning = false;
            BoardHAL::display.clearBuffer();
            BoardHAL::display.setFont(u8g2_font_6x10_tr);
            BoardHAL::display.drawStr(0, 15, BoardHAL::getBoardName());
            String ipStr = "IP: " + WiFi.localIP().toString();
            BoardHAL::display.drawStr(0, 30, ipStr.c_str());
            BoardHAL::display.drawStr(0, 45, "Radio: LISTENING");
            BoardHAL::display.sendBuffer();
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

void IoHomeWebSniffer::handleRoot() {
    String controlsHtml = "";
    int activeCount = 0;
    for (int i = 0; i < 5; i++) {
        if (devices[i].active) {
            activeCount++;
            char buf[1024];
            snprintf(buf, sizeof(buf),
                "<b>Channel %d:</b> <input type='text' id='desc_%d' value='%s' maxlength='31' placeholder='Unnamed Device'> "
                "<button class=\"btn-small\" onclick=\"fetch('/name?d=%d&n='+encodeURIComponent(document.getElementById('desc_%d').value))\">Save Name</button><br>"
                "<span style='font-size:12px;color:#aaa;'>Src: %02X%02X%02X | Dest: %02X%02X%02X</span><br>"
                "<button class=\"btn\" onclick=\"fetch('/cmd?c=U&d=%d')\">UP</button>"
                "<button class=\"btn\" onclick=\"fetch('/cmd?c=S&d=%d')\">MY</button>"
                "<button class=\"btn\" onclick=\"fetch('/cmd?c=D&d=%d')\">DOWN</button><br><br>",
                i + 1, i, devices[i].description, i, i,
                devices[i].sourceMac.n0, devices[i].sourceMac.n1, devices[i].sourceMac.n2,
                devices[i].destMac.n0, devices[i].destMac.n1, devices[i].destMac.n2,
                i, i, i);
            controlsHtml += buf;
        }
    }
    if (activeCount == 0) {
        controlsHtml = "<p style='color:#ff0;'>No devices enrolled yet. Use your remote to send a 1-Way Key Transfer.</p>";
    }

    String html = R"HTML(<html><head><title>IoHome Sniffer</title>
    <style>body{font-family: monospace; background: #121212; color: #0f0; padding: 20px;} .pkt{border-bottom: 1px solid #333; padding: 5px; margin-bottom: 5px;}
    .btn{background:#333;color:#0f0;border:1px solid #0f0;padding:12px 24px;margin:5px;cursor:pointer;font-family:monospace;font-size:16px;font-weight:bold;}
    .btn:hover{background:#0f0;color:#121212;}
    .btn-small{background:#333;color:#0f0;border:1px solid #0f0;padding:4px 8px;cursor:pointer;font-family:monospace;font-weight:bold;}
    .btn-small:hover{background:#0f0;color:#121212;}
    input[type=text]{background:#222;color:#0f0;border:1px solid #0f0;padding:4px;font-family:monospace;margin-right:5px;}
    .controls{margin-bottom: 20px; padding-bottom: 20px; border-bottom: 2px solid #0f0;}
    </style>
    </head><body><h2>IoHome Packet Sniffer</h2>
    <div class="controls">)HTML" + controlsHtml + R"HTML(</div><p>Waiting for packets...</p><div id="log"></div>
    <script>setInterval(() => { fetch('/packets').then(r => r.text()).then(t => {
    if(t) document.getElementById('log').innerHTML = t + document.getElementById('log').innerHTML;
    });}, 1000);</script></body></html>)HTML";
    _server.send(200, "text/html", html);
}

void IoHomeWebSniffer::handlePackets() {
    _server.send(200, "text/html", _newPackets);
    _newPackets = ""; // Clear buffer after sending to prevent memory growth
}

void IoHomeWebSniffer::begin() {
    WiFi.onEvent(sysProvEvent);

    WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM, NETWORK_PROV_SECURITY_1, "iown1234", "PROV_IoHome");

    // Using lambdas to bind class methods to the WebServer handlers
    _server.on("/", [this]() { handleRoot(); });
    _server.on("/packets", [this]() { handlePackets(); });
    _server.on("/cmd", [this]() {
        if (_server.hasArg("c")) {
            if (_server.hasArg("d")) {
                webCommandDevice = _server.arg("d").toInt();
            }
            webCommandTarget = _server.arg("c")[0];
        }
        _server.send(200, "text/plain", "Command Queued");
    });
    _server.on("/name", [this]() {
        if (_server.hasArg("d") && _server.hasArg("n")) {
            uint8_t devIdx = _server.arg("d").toInt();
            if (devIdx < 5) {
                String n = _server.arg("n");
                strncpy(ioNode.getProfiles()[devIdx].description, n.c_str(), 31);
                ioNode.getProfiles()[devIdx].description[31] = '\0';
                saveConfiguration(); // Syncs ioNode -> devices array -> NVRAM
                Serial.printf("\n>>> UPDATED NAME FOR CH %d to '%s' <<<\n", devIdx + 1, ioNode.getProfiles()[devIdx].description);
            }
        }
        _server.send(200, "text/plain", "Name Saved");
    });
    _server.begin();
}

void IoHomeWebSniffer::loop() {
    _server.handleClient();

    // Self-Healing Wi-Fi Logic: If the board has stale/dead credentials from an old project,
    // it will try to connect forever and never show the QR code.
    // If 15 seconds pass without connecting, wipe the dead credentials and reboot into setup mode!
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

void IoHomeWebSniffer::appendRawPacket(float freq, float rssi, size_t len, const uint8_t* data) {
    String rawHtml = "<div class='pkt'>[RAW] Freq: " + String(freq) + " MHz | RSSI: " + String(rssi) + " | Len: " + String(len) + "<br>Data: ";
    char hexBuf[4];
    for (size_t i = 0; i < len; i++) {
        snprintf(hexBuf, sizeof(hexBuf), "%02X ", data[i]);
        rawHtml += hexBuf;
    }
    rawHtml += "</div>";
    // Prepend so packets display in reverse chronological order within the batch
    _newPackets = rawHtml + _newPackets;
    if (_newPackets.length() > 8192) _newPackets = ""; // Memory protection
}

void IoHomeWebSniffer::appendDecodedPacket(uint32_t frameCount, const IoHomeFrame_t& frame, bool isAuth) {
    String actionStr = "";
    if (frame.commandId == 0x00 && isAuth && frame.payload.size() >= 4) {
        uint16_t action = (frame.payload[2] << 8) | frame.payload[3];
        if (action == IOHOME_ACTION_UP) actionStr = " | <span style='color:#ff0;'>Action: UP / OPEN</span>";
        else if (action == IOHOME_ACTION_DOWN) actionStr = " | <span style='color:#ff0;'>Action: DOWN / CLOSE</span>";
        else if (action == IOHOME_ACTION_MY) actionStr = " | <span style='color:#ff0;'>Action: MY / STOP</span>";
        else actionStr = " | <span style='color:#ff0;'>Action: UNKNOWN</span>";

        if (frame.payload.size() >= 6) {
            uint16_t param = (frame.payload[4] << 8) | frame.payload[5];
            char paramBuf[32];
            snprintf(paramBuf, sizeof(paramBuf), " (Param: 0x%04X)", param);
            actionStr += paramBuf;
        }
    } else if (frame.commandId == 0x30) {
        actionStr = " | <span style='color:#ff0;'>Action: 1-WAY KEY TRANSFER</span>";
    }

    char parsedHtml[1024];
    snprintf(parsedHtml, sizeof(parsedHtml),
             "<div class='pkt' style='color:#0ff;'><b>#%lu DECODED IOHOME FRAME</b><br>Command: 0x%02X | Source: %02X%02X%02X | Dest: %02X%02X%02X | Auth: %s%s</div>",
             frameCount, frame.commandId,
             frame.sourceMac.n0, frame.sourceMac.n1, frame.sourceMac.n2,
             frame.destMac.n0, frame.destMac.n1, frame.destMac.n2,
             isAuth ? "SUCCESS" : "FAILED", actionStr.c_str());
    // Prepend so packets display in reverse chronological order within the batch
    _newPackets = String(parsedHtml) + _newPackets;
    if (_newPackets.length() > 8192) _newPackets = ""; // Memory protection
}
