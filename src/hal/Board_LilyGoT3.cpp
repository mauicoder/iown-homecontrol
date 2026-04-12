#include "BoardHAL.h"
#if defined(LILYGO_T3_V16)

#include <SPI.h>

// --- LILYGO T3 V1.6.1 PIN MAPPING ---
#define HW_LED             25
#define LORA_NSS           18
#define LORA_DIO0          26
#define LORA_NRST          23
#define LORA_BUSY          RADIOLIB_NC
#define LORA_SCK           5
#define LORA_MISO          19
#define LORA_MOSI          27
#define OLED_SDA           21
#define OLED_SCL           22
#ifdef OLED_RST
#undef OLED_RST
#endif
#define OLED_RST           U8X8_PIN_NONE

namespace BoardHAL {
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

    static Module* mod = new Module(LORA_NSS, LORA_DIO0, LORA_NRST, LORA_BUSY);
    static SX1276 sx1276 = mod;
    PhysicalLayer* radio = &sx1276; // Upcast to generic radio layer

    void initPower() {
        pinMode(HW_LED, OUTPUT);
        digitalWrite(HW_LED, LOW);
    }

    const char* getBoardName() { return "LilyGO T3 IoHome"; }

    bool initRadioProtocol(float targetFreq, float bitrate, float freqDev, float rxBw, uint16_t preambleLen, uint8_t fixedPayloadLen, uint8_t* syncWord, uint8_t syncWordLen, void (*isr)(void)) {
        pinMode(LORA_NRST, OUTPUT); digitalWrite(LORA_NRST, LOW); delay(50); digitalWrite(LORA_NRST, HIGH); delay(200);

        SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI);

        int16_t state;

        // Widen RX Bandwidth to 250.0 kHz to compensate for LilyGo's extreme crystal drift
        // and the wide deviation of the io-homecontrol FSK signal.
        float widenedRxBw = 250.0;
        state = sx1276.beginFSK(targetFreq, bitrate, freqDev, widenedRxBw);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("beginFSK failed: %d\n", state); return false; }

        // LilyGO specific tuning
        state = sx1276.setDataShaping(RADIOLIB_SHAPING_NONE);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setDataShaping failed: %d\n", state); return false; }

        state = sx1276.setPreambleLength(preambleLen);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setPreambleLength failed: %d\n", state); return false; }
        state = sx1276.setCRC(false);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setCRC failed: %d\n", state); return false; }

        state = sx1276.fixedPacketLengthMode(fixedPayloadLen);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("fixedPacketLengthMode failed: %d\n", state); return false; }
        state = sx1276.setSyncWord(syncWord, syncWordLen);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setSyncWord failed: %d\n", state); return false; }

        sx1276.setDio0Action(isr, RISING);
        return true;
    }

    int16_t startReceive() {
        // 1. Let RadioLib configure its internal state machine first
        int16_t state = sx1276.startReceive();

        // 2. Apply our protocol-specific patches AFTER so they aren't overwritten
        mod->SPIsetRegValue(0x1F, 0x00); // Disable Preamble Detector
        mod->SPIsetRegValue(0x29, 0xB4); // Set RSSI Threshold to -90 dBm

        // RegRxConfig (0x0D): AfcAutoOn = 0 (CRITICAL), AgcAutoOn = 1, RxTrigger = 001 (RSSI) -> Binary 0000 1001 = 0x09
        // AFC must be OFF because the unbalanced 0xFF preamble actively detunes the hardware.
        mod->SPIsetRegValue(0x0D, 0x09);

        uint8_t syncConfig = mod->SPIgetRegValue(0x27);
        mod->SPIsetRegValue(0x27, syncConfig & 0x7F); // Disable AutoRxRestartOn
        return state;
    }

    int16_t readData(uint8_t* data, size_t& len) {
        size_t packetLen = sx1276.getPacketLength();
        // The SX1276 strips the physical sync word (0x57FD99).
        // We restore these raw physical bits at the beginning of the buffer
        // so the software UART stream parser can successfully decode them into 0xFF 0x33.
        data[0] = 0x57;
        data[1] = 0xFD;
        data[2] = 0x99;
        int16_t state = sx1276.readData(data + 3, packetLen);
        len = packetLen + 3;
        return state;
    }

    float getInstantaneousRSSI() { return sx1276.getRSSI(false, false); }
    void setLed(bool state) { digitalWrite(HW_LED, state ? HIGH : LOW); }
}
#endif
