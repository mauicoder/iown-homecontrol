#pragma once

#include <Arduino.h>
#include "IoHome.h"

namespace UIManager {
    void begin();
    void loop();
    void drawBootScreen();
    void drawReadyScreen(const String& status);
    void drawPacketHistory(uint32_t frameCount, const IoHomeFrame_t& frame);
    bool isProvisioning();
}
