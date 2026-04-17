#include "ConfigManager.h"
#include <Preferences.h>

namespace ConfigManager {
    IoHomeProfile devices[5];
    MqttConfig mqttConfig;
    static Preferences preferences;

    void load(IoHomeNode& ioNode) {
        preferences.begin("iohome", true); // true = read-only mode

        size_t len = preferences.getBytesLength("devices");
        uint8_t* oldData = nullptr;
        bool needsMigration = false;

        if (len == sizeof(devices)) {
            preferences.getBytes("devices", devices, sizeof(devices));
            Serial.println("Loaded Multi-Channel Device Profiles from NVRAM.");
        } else if (len > 0 && len < sizeof(devices)) {
            Serial.println("Migrating old Device Profiles to new format...");
            oldData = new uint8_t[len];
            preferences.getBytes("devices", oldData, len);
            needsMigration = true;
        } else {
            Serial.println("No Device Profiles in config. Using empty array.");
            memset(devices, 0, sizeof(devices));
        }

        // Load MQTT Configuration
        size_t mqttLen = preferences.getBytesLength("mqtt");
        if (mqttLen > 0 && mqttLen <= sizeof(MqttConfig)) {
            preferences.getBytes("mqtt", &mqttConfig, sizeof(MqttConfig));
            Serial.println("Loaded MQTT Configuration from NVRAM.");
        } else {
            memset(&mqttConfig, 0, sizeof(MqttConfig));
            mqttConfig.port = 1883;
            strncpy(mqttConfig.server, "homeassistant.local", sizeof(mqttConfig.server) - 1);
            strncpy(mqttConfig.baseTopic, "iown", sizeof(mqttConfig.baseTopic) - 1);
            Serial.println("No MQTT Configuration found. Using defaults.");
        }

        preferences.end(); // All read operations are done, close preferences.

        if (needsMigration) {
            size_t oldProfileSize = len / 5;
            memset(devices, 0, sizeof(devices));
            for (int i = 0; i < 5; i++) {
                memcpy(&devices[i], oldData + (i * oldProfileSize), oldProfileSize);
            }
            delete[] oldData;
            ioNode.loadProfiles(devices, 5); // Load into RAM before save
            saveDevices(ioNode); // Save new format
        } else {
            ioNode.loadProfiles(devices, 5);
        }

        // Print the loaded keys and MACs to the console
        int activeCount = 0;
        Serial.println(F("\n--- ENROLLED DEVICES / KEYS ---"));
        for (int i = 0; i < 5; i++) {
            if (devices[i].active) {
                activeCount++;
                Serial.printf("Slot %d: [%s] Source MAC: %02X%02X%02X | Dest MAC: %02X%02X%02X | Seq: %u\n",
                    i + 1,
                    strlen(devices[i].description) > 0 ? devices[i].description : "Unnamed",
                    devices[i].sourceMac.n0, devices[i].sourceMac.n1, devices[i].sourceMac.n2,
                    devices[i].destMac.n0, devices[i].destMac.n1, devices[i].destMac.n2,
                    devices[i].seqCounter);
            }
        }
        if (activeCount == 0) Serial.println(F("No keys enrolled yet. Use your remote to send a 1-Way Key Transfer (0x30)."));
        Serial.printf("Total enrolled keys: %d / 5\n", activeCount);
        Serial.println(F("-------------------------------\n"));
    }

    void saveDevices(IoHomeNode& ioNode) {
        // Sync active state from ioNode to catch counter increments
        memcpy(devices, ioNode.getProfiles(), sizeof(devices));
        preferences.begin("iohome", false); // false = read/write mode
        preferences.putBytes("devices", devices, sizeof(devices));
        preferences.end();
    }

    void saveMqtt() {
        preferences.begin("iohome", false); // false = read/write mode
        size_t written = preferences.putBytes("mqtt", &mqttConfig, sizeof(MqttConfig));
        Serial.printf("[NVRAM] Saved MQTT Config: %zu bytes\n", written);
        preferences.end();
    }
}
