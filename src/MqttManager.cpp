#include "MqttManager.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "hal/BoardHAL.h"
#include "ConfigManager.h"

extern IoHomeNode ioNode;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

uint32_t lastMqttReconnectAttempt = 0;
MqttConfig currentConfig;

namespace MqttManager {

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    Serial.printf("\n[MQTT] Command Received: Topic: %s | Payload: %s\n", topic, payloadStr.c_str());

    // Dynamic topic matching: {baseTopic}/cover/{index}/set
    String expectedPrefix = String(currentConfig.baseTopic) + "/cover/";
    if (topicStr.startsWith(expectedPrefix) && topicStr.endsWith("/set")) {
        int nextSlash = topicStr.indexOf('/', expectedPrefix.length());
        if (nextSlash != -1) {
            String indexStr = topicStr.substring(expectedPrefix.length(), nextSlash);
            uint8_t channelIndex = indexStr.toInt();

            if (channelIndex < 5 && ConfigManager::devices[channelIndex].active) {
                BoardHAL::radio->standby(); // Pause receiving
                int16_t state = RADIOLIB_ERR_NONE;

                if (payloadStr == "OPEN") {
                    state = ioNode.sendButton(IOHOME_ACTION_UP, channelIndex);
                    if (state == RADIOLIB_ERR_NONE) publishState(channelIndex, "open");
                } else if (payloadStr == "CLOSE") {
                    state = ioNode.sendButton(IOHOME_ACTION_DOWN, channelIndex);
                    if (state == RADIOLIB_ERR_NONE) publishState(channelIndex, "closed");
                } else if (payloadStr == "STOP") {
                    state = ioNode.sendButton(IOHOME_ACTION_MY, channelIndex);
                    if (state == RADIOLIB_ERR_NONE) publishState(channelIndex, "stopped");
                }

                if (state == RADIOLIB_ERR_NONE) {
                    Serial.println("    [MQTT] Transmission Successful");
                    ConfigManager::saveDevices(ioNode); // Save incremented sequence counter
                } else {
                    Serial.printf("    [MQTT] Transmission Failed: %d\n", state);
                }
                BoardHAL::startReceive(); // Resume receiving
            }
        }
    }
}

void publishDiscovery() {
    for (int i = 0; i < 5; i++) {
        if (ConfigManager::devices[i].active) {
            String uniqueId = "iown_awning_" + String(i);
            String name = strlen(ConfigManager::devices[i].description) > 0 ? String(ConfigManager::devices[i].description) : ("IoHome Awning " + String(i + 1));
            String commandTopic = String(currentConfig.baseTopic) + "/cover/" + String(i) + "/set";
            String stateTopic = String(currentConfig.baseTopic) + "/cover/" + String(i) + "/state";
            String discoveryTopic = "homeassistant/cover/" + uniqueId + "/config";

            // Home Assistant MQTT Discovery JSON for a Cover entity
            String payload = "{";
            payload += "\"name\": \"" + name + "\",";
            payload += "\"command_topic\": \"" + commandTopic + "\",";
            payload += "\"state_topic\": \"" + stateTopic + "\",";
            payload += "\"device_class\": \"awning\",";
            payload += "\"optimistic\": true,";
            payload += "\"unique_id\": \"" + uniqueId + "\"";
            payload += "}";

            Serial.printf("    [MQTT] Publishing HA Discovery for Awning %d\n", i + 1);
            mqttClient.publish(discoveryTopic.c_str(), payload.c_str(), true); // true = retain message
        }
    }
}

void reconnect() {
    if (millis() - lastMqttReconnectAttempt > 5000) {
        lastMqttReconnectAttempt = millis();

        if (strlen(currentConfig.server) == 0) {
            Serial.println("[MQTT] Not configured. Skipping connection attempt.");
            return;
        }

        Serial.print("[MQTT] Attempting connection to ");
        Serial.print(currentConfig.server);
        Serial.print("... ");

        String clientId = "IoHome-ESP32-" + String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str(), currentConfig.user, currentConfig.password)) {
            Serial.println("Connected!");
            publishDiscovery(); // Broadcast Home Assistant auto-discovery configs

            String subTopic = String(currentConfig.baseTopic) + "/cover/+/set";
            mqttClient.subscribe(subTopic.c_str());
        } else {
            Serial.print("Failed, rc=");
            Serial.print(mqttClient.state());
            Serial.print(" - ");
            switch (mqttClient.state()) {
                case -4: Serial.println("Connection timeout"); break;
                case -3: Serial.println("Connection lost"); break;
                case -2: Serial.println("Connect failed (check server address/port)"); break;
                case 1:  Serial.println("Bad protocol version"); break;
                case 2:  Serial.println("Client ID rejected"); break;
                case 3:  Serial.println("Server unavailable"); break;
                case 4:  Serial.println("Bad credentials (check username/password)"); break;
                case 5:  Serial.println("Not authorized"); break;
                default: Serial.println("Unknown error"); break;
            }
        }
    }
}

void begin(const MqttConfig& config) {
    currentConfig = config;
    if (strlen(currentConfig.server) > 0) {
        mqttClient.setServer(currentConfig.server, currentConfig.port);
    }
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(512); // Increase buffer size for large Discovery JSONs
}

void loop() {
    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
        reconnect();
    }
    if (mqttClient.connected()) {
        mqttClient.loop();
    }
}

void publishState(uint8_t profileIndex, const char* state) {
    if (mqttClient.connected()) {
        String stateTopic = String(currentConfig.baseTopic) + "/cover/" + String(profileIndex) + "/state";
        mqttClient.publish(stateTopic.c_str(), state);
    }
}

void disconnect() {
    mqttClient.disconnect();
}

bool isConnected() {
    return mqttClient.connected();
}

} // namespace MqttManager
