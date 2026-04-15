#pragma once
#include <Arduino.h>
#include "IoHome.h"
#include "MqttManager.h"

namespace ConfigManager {
    extern IoHomeProfile devices[5];
    extern MqttConfig mqttConfig;

    void load(IoHomeNode& ioNode);
    void saveDevices(IoHomeNode& ioNode);
    void saveMqtt();
}
