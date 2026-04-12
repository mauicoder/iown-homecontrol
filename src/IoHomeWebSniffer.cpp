#include "IoHomeWebSniffer.h"

IoHomeWebSniffer::IoHomeWebSniffer() : _server(80) {}

void IoHomeWebSniffer::sysProvEvent(arduino_event_t *sys_event) {
    switch (sys_event->event_id) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("\n>>> Wi-Fi Connected! IP: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_PROV_START:
            Serial.println("\n>>> Wi-Fi Provisioning Started.");
            Serial.println(">>> Use the 'ESP BLE Prov' App. Device: IoHomeSniffer, PoP: iown1234");
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            Serial.println("\n>>> Provisioning Successful!");
            break;
        default:
            break;
    }
}

void IoHomeWebSniffer::handleRoot() {
    String html = R"(<html><head><title>IoHome Sniffer</title>
    <style>body{font-family: monospace; background: #121212; color: #0f0; padding: 20px;} .pkt{border-bottom: 1px solid #333; padding: 5px; margin-bottom: 5px;}</style>
    </head><body><h2>IoHome Packet Sniffer</h2><p>Waiting for packets...</p><div id="log"></div>
    <script>setInterval(() => { fetch('/packets').then(r => r.text()).then(t => {
    if(t) document.getElementById('log').innerHTML = t + document.getElementById('log').innerHTML;
    });}, 1000);</script></body></html>)";
    _server.send(200, "text/html", html);
}

void IoHomeWebSniffer::handlePackets() {
    _server.send(200, "text/html", _newPackets);
    _newPackets = ""; // Clear buffer after sending to prevent memory growth
}

void IoHomeWebSniffer::begin() {
    WiFi.onEvent(sysProvEvent);
    WiFiProv.beginProvision(NETWORK_PROV_SCHEME_BLE, NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM, NETWORK_PROV_SECURITY_1, "iown1234", "IoHomeSniffer");

    // Using lambdas to bind class methods to the WebServer handlers
    _server.on("/", [this]() { handleRoot(); });
    _server.on("/packets", [this]() { handlePackets(); });
    _server.begin();
}

void IoHomeWebSniffer::loop() {
    _server.handleClient();
}

void IoHomeWebSniffer::appendRawPacket(float freq, float rssi, size_t len, const uint8_t* data) {
    String rawHtml = "<div class='pkt'>[RAW] Freq: " + String(freq) + " MHz | RSSI: " + String(rssi) + " | Len: " + String(len) + "<br>Data: ";
    char hexBuf[4];
    for (size_t i = 0; i < len; i++) {
        snprintf(hexBuf, sizeof(hexBuf), "%02X ", data[i]);
        rawHtml += hexBuf;
    }
    rawHtml += "</div>";
    _newPackets += rawHtml;
    if (_newPackets.length() > 8192) _newPackets = ""; // Memory protection
}

void IoHomeWebSniffer::appendDecodedPacket(uint32_t frameCount, const IoHomeFrame_t& frame, bool isAuth) {
    char parsedHtml[512];
    snprintf(parsedHtml, sizeof(parsedHtml),
             "<div class='pkt' style='color:#0ff;'><b>#%lu DECODED IOHOME FRAME</b><br>Command: 0x%02X | Source: %02X%02X%02X | Dest: %02X%02X%02X | Auth: %s</div>",
             frameCount, frame.commandId,
             frame.sourceMac.n0, frame.sourceMac.n1, frame.sourceMac.n2,
             frame.destMac.n0, frame.destMac.n1, frame.destMac.n2,
             isAuth ? "SUCCESS" : "FAILED");
    _newPackets += String(parsedHtml);
    if (_newPackets.length() > 8192) _newPackets = ""; // Memory protection
}
