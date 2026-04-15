#pragma once

#include <Arduino.h>
#include "IoHome.h"

struct MqttConfig {
    char server[64];
    uint16_t port;
    char user[32];
    char password[64];
    char baseTopic[32];
};

namespace MqttManager {
    void begin(const MqttConfig& config);
    void loop();
    void publishState(uint8_t profileIndex, const char* state);
    void publishDiscovery();
    void disconnect(); // Forces a reconnect when settings are updated
    bool isConnected(); // Returns true if connected to broker
}
