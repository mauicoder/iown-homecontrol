#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "IoHome.h"

class IoHomeWebSniffer {
private:
    WebServer _server;
    String _newPackets;

    void handleRoot();
    void handlePackets();
public:
    IoHomeWebSniffer();
    void begin();
    void loop();
    void appendRawPacket(float freq, float rssi, size_t len, const uint8_t* data);
    void appendDecodedPacket(uint32_t frameCount, const IoHomeFrame_t& frame, bool isAuth);
};
