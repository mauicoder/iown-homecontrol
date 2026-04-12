#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>

namespace BoardHAL {
    // Globally accessible hardware objects
    extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;
    extern PhysicalLayer* radio;

    // Hardware-specific initialization routines
    void initPower();
    const char* getBoardName();
    bool initRadioProtocol(float targetFreq, float bitrate, float freqDev, float rxBw, uint16_t preambleLen, uint8_t fixedPayloadLen, uint8_t* syncWord, uint8_t syncWordLen, void (*isr)(void));
    int16_t startReceive();
    int16_t readData(uint8_t* data, size_t& len);
    float getInstantaneousRSSI();
    void setLed(bool state);
}
