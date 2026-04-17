#include "IoHomeWebSniffer.h"
#include "IoHome.h"
#include "MqttManager.h"
#include "ConfigManager.h"

volatile char webCommandTarget = 0; // Global variable to pass web commands to the main loop
volatile uint8_t webCommandDevice = 0;

extern IoHomeNode ioNode; // Needed to update the active profile in RAM

IoHomeWebSniffer::IoHomeWebSniffer() : _server(80) {}

void IoHomeWebSniffer::handleRoot() {
    String controlsHtml = "";
    int activeCount = 0;
    for (int i = 0; i < 5; i++) {
        if (ConfigManager::devices[i].active) {
            activeCount++;
            char buf[1024];
            snprintf(buf, sizeof(buf),
                "<b>Channel %d:</b> <input type='text' id='desc_%d' value='%s' maxlength='31' placeholder='Unnamed Device'> "
                "<button class=\"btn-small\" onclick=\"fetch('/name?d=%d&n='+encodeURIComponent(document.getElementById('desc_%d').value))\">Save Name</button><br>"
                "<span style='font-size:12px;color:#aaa;'>Src: %02X%02X%02X | Dest: %02X%02X%02X</span><br>"
                "<button class=\"btn\" onclick=\"fetch('/cmd?c=U&d=%d')\">UP</button>"
                "<button class=\"btn\" onclick=\"fetch('/cmd?c=S&d=%d')\">MY</button>"
                "<button class=\"btn\" onclick=\"fetch('/cmd?c=D&d=%d')\">DOWN</button>"
                "<button class=\"btn\" style=\"background:#005500;\" onclick=\"fetch('/cmd?c=P&d=%d')\">POLL</button><br><br>",
                i + 1, i, ConfigManager::devices[i].description, i, i,
                ConfigManager::devices[i].sourceMac.n0, ConfigManager::devices[i].sourceMac.n1, ConfigManager::devices[i].sourceMac.n2,
                ConfigManager::devices[i].destMac.n0, ConfigManager::devices[i].destMac.n1, ConfigManager::devices[i].destMac.n2,
                i, i, i, i);
            controlsHtml += buf;
        }
    }
    if (activeCount == 0) {
        controlsHtml = "<p style='color:#ff0;'>No devices enrolled yet. Use your remote to send a 1-Way Key Transfer.</p>";
    }

    const char* mqttStatus = MqttManager::isConnected() ? "<span style='color:#0f0;'> (Connected)</span>" : "<span style='color:#f00;'> (Disconnected)</span>";

    char mqttBuf[1024];
    snprintf(mqttBuf, sizeof(mqttBuf),
        "<h3>MQTT Configuration%s</h3>"
        "Server: <input type='text' id='mqtt_s' value='%s' size='20'> "
        "Port: <input type='text' id='mqtt_p' value='%d' size='5'><br><br>"
        "User: <input type='text' id='mqtt_u' value='%s' size='15'> "
        "Pass: <input type='password' id='mqtt_w' value='%s' size='15'><br><br>"
        "Topic: <input type='text' id='mqtt_t' value='%s' size='15'> "
        "<button class=\"btn-small\" onclick=\"fetch('/mqttcfg?s='+encodeURIComponent(document.getElementById('mqtt_s').value)+'&p='+encodeURIComponent(document.getElementById('mqtt_p').value)+'&u='+encodeURIComponent(document.getElementById('mqtt_u').value)+'&w='+encodeURIComponent(document.getElementById('mqtt_w').value)+'&t='+encodeURIComponent(document.getElementById('mqtt_t').value)).then(r => r.text()).then(t => alert(t))\">Save MQTT</button><br><br>",
        mqttStatus, ConfigManager::mqttConfig.server, ConfigManager::mqttConfig.port, ConfigManager::mqttConfig.user, ConfigManager::mqttConfig.password, ConfigManager::mqttConfig.baseTopic);
    String mqttHtml = mqttBuf;

    String html = R"HTML(<html><head><title>IoHome Sniffer</title>
    <style>body{font-family: monospace; background: #121212; color: #0f0; padding: 20px;} .pkt{border-bottom: 1px solid #333; padding: 5px; margin-bottom: 5px;}
    .btn{background:#333;color:#0f0;border:1px solid #0f0;padding:12px 24px;margin:5px;cursor:pointer;font-family:monospace;font-size:16px;font-weight:bold;}
    .btn:hover{background:#0f0;color:#121212;}
    .btn-small{background:#333;color:#0f0;border:1px solid #0f0;padding:4px 8px;cursor:pointer;font-family:monospace;font-weight:bold;}
    .btn-small:hover{background:#0f0;color:#121212;}
    input[type=text], input[type=password]{background:#222;color:#0f0;border:1px solid #0f0;padding:4px;font-family:monospace;margin-right:5px;}
    .controls{margin-bottom: 20px; padding-bottom: 20px; border-bottom: 2px solid #0f0;}
    </style>
    </head><body><h2>IoHome Packet Sniffer</h2>
    <div class="controls">)HTML" + controlsHtml + R"HTML(</div><div class="controls">)HTML" + mqttHtml + R"HTML(</div><p>Waiting for packets...</p><div id="log"></div>
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
                ConfigManager::saveDevices(ioNode); // Syncs ioNode -> NVRAM
                Serial.printf("\n>>> UPDATED NAME FOR CH %d to '%s' <<<\n", devIdx + 1, ioNode.getProfiles()[devIdx].description);
            }
        }
        _server.send(200, "text/plain", "Name Saved");
    });
    _server.on("/mqttcfg", [this]() {
        if (_server.hasArg("s")) {
            // Clear the struct to safely remove any old longer strings
            memset(&ConfigManager::mqttConfig, 0, sizeof(MqttConfig));

            strncpy(ConfigManager::mqttConfig.server, _server.arg("s").c_str(), sizeof(ConfigManager::mqttConfig.server) - 1);
            ConfigManager::mqttConfig.port = _server.hasArg("p") ? _server.arg("p").toInt() : 1883;
            strncpy(ConfigManager::mqttConfig.user, _server.arg("u").c_str(), sizeof(ConfigManager::mqttConfig.user) - 1);
            strncpy(ConfigManager::mqttConfig.password, _server.arg("w").c_str(), sizeof(ConfigManager::mqttConfig.password) - 1);

            String topicArg = _server.arg("t");
            if (topicArg.length() > 0) {
                strncpy(ConfigManager::mqttConfig.baseTopic, topicArg.c_str(), sizeof(ConfigManager::mqttConfig.baseTopic) - 1);
            } else {
                strncpy(ConfigManager::mqttConfig.baseTopic, "iown", sizeof(ConfigManager::mqttConfig.baseTopic) - 1);
            }

            ConfigManager::saveMqtt();

            MqttManager::disconnect(); // Force a clean disconnect
            MqttManager::begin(ConfigManager::mqttConfig); // Re-init with new settings
        }
        _server.send(200, "text/plain", "MQTT Saved");
    });
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
    } else if (frame.commandId == 0x54) {
        actionStr = " | <span style='color:#0f0;'>Action: POLL STATUS (Get Info 1)</span>";
    } else if (frame.commandId == 0x55) {
        actionStr = " | <span style='color:#0ff;'>Action: STATUS REPLY (Info 1 Answer)</span>";
    }

    bool isFromAwning = false;
    for (int i = 0; i < 5; i++) {
        if (ConfigManager::devices[i].active && memcmp(&ConfigManager::devices[i].destMac, &frame.sourceMac, 3) == 0) {
            isFromAwning = true;
            break;
        }
    }
    if (isFromAwning) {
        actionStr += " | <span style='color:#f0f;'>[AWNING REPLY]</span>";
    }

    String payloadStr = "Payload: ";
    for (size_t i = 0; i < frame.payload.size(); i++) {
        char hexBuf[4];
        snprintf(hexBuf, sizeof(hexBuf), "%02X ", frame.payload[i]);
        payloadStr += hexBuf;
    }

    char parsedHtml[1024];
    snprintf(parsedHtml, sizeof(parsedHtml),
             "<div class='pkt' style='color:#0ff;'><b>#%lu DECODED IOHOME FRAME</b><br>Command: 0x%02X | Source: %02X%02X%02X | Dest: %02X%02X%02X | Auth: %s%s<br><span style='color:#aaa;'>%s</span></div>",
             frameCount, frame.commandId,
             frame.sourceMac.n0, frame.sourceMac.n1, frame.sourceMac.n2,
             frame.destMac.n0, frame.destMac.n1, frame.destMac.n2,
             isAuth ? "SUCCESS" : "FAILED", actionStr.c_str(), payloadStr.c_str());
    // Prepend so packets display in reverse chronological order within the batch
    _newPackets = String(parsedHtml) + _newPackets;
    if (_newPackets.length() > 8192) _newPackets = ""; // Memory protection
}
