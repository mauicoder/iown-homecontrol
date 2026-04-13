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
    static float lastPacketRssi = -127.0f;
    static bool isReceiving = false;

    void initPower() {
        pinMode(HW_LED, OUTPUT);
        digitalWrite(HW_LED, LOW);
    }

    const char* getBoardName() { return "LilyGO T3 IoHome"; }

    bool initRadioProtocol(float targetFreq, float bitrate, float freqDev, float rxBw, uint16_t preambleLen, uint8_t fixedPayloadLen, uint8_t* syncWord, uint8_t syncWordLen, void (*isr)(void)) {
        pinMode(LORA_NRST, OUTPUT); digitalWrite(LORA_NRST, LOW); delay(50); digitalWrite(LORA_NRST, HIGH); delay(200);

        SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI);

        int16_t state;

        // Use 125.0 kHz to allow for crystal drift, but narrow enough to prevent
        // strong adjacent-channel signals (-30 dBm) from bleeding over and halting the hopper.
        float widenedRxBw = 125.0;
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
        // Rely safely on RadioLib's tested internal state machine.
        // This implicitly uses standard Preamble Detection, AFC, and strict Sync Word validation,
        // eliminating false positives and maintaining state consistency.
        int16_t state = sx1276.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
            isReceiving = true;
        }
        return state;
    }

    int16_t readData(uint8_t* data, size_t& len) {
        // Read RSSI safely via RadioLib API *before* readData puts the chip in Standby.
        // skipReceive = true prevents RadioLib from accidentally forcing a state change.
        lastPacketRssi = sx1276.getRSSI(false, true);

        size_t packetLen = sx1276.getPacketLength();
        int16_t state = sx1276.readData(data, packetLen);
        len = packetLen;

        isReceiving = false; // RadioLib automatically transitions to Standby after readData
        return state;
    }

    float getInstantaneousRSSI() {
        if (isReceiving) {
            // Live reading for Clear Channel Assessment (LBT)
            return sx1276.getRSSI(false, true);
        }
        return lastPacketRssi;
    }
    void setLed(bool state) { digitalWrite(HW_LED, state ? HIGH : LOW); }
}
#endif
