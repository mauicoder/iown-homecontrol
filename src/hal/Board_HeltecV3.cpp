#include "BoardHAL.h"
#if !defined(LILYGO_T3_V16)

#include <SPI.h>

// --- HELTEC V3.2 PIN MAPPING ---
#define HW_VEXT            36
#define HW_LED             35
#define LORA_NSS           8
#define LORA_DIO1          14
#define LORA_NRST          12
#define LORA_BUSY          13
#define LORA_SCK           9
#define LORA_MISO          11
#define LORA_MOSI          10
#define OLED_SDA           17
#define OLED_SCL           18
#define OLED_RST           21

namespace BoardHAL {
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

    static SX1262 sx1262 = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);
    PhysicalLayer* radio = &sx1262; // Upcast to generic radio layer

    void initPower() {
        pinMode(HW_VEXT, OUTPUT);
        digitalWrite(HW_VEXT, LOW); // Pull LOW to power the SX1262 and OLED
        pinMode(HW_LED, OUTPUT);
        digitalWrite(HW_LED, LOW);
    }

    const char* getBoardName() { return "Heltec V3.2 IoHome"; }

    bool initRadioProtocol(float targetFreq, float bitrate, float freqDev, float rxBw, uint16_t preambleLen, uint8_t fixedPayloadLen, uint8_t* syncWord, uint8_t syncWordLen, void (*isr)(void)) {
        pinMode(LORA_NRST, OUTPUT); digitalWrite(LORA_NRST, LOW); delay(50); digitalWrite(LORA_NRST, HIGH); delay(200);

        SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI);

        int16_t state;
        state = sx1262.beginFSK(targetFreq, bitrate, freqDev, rxBw);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("beginFSK failed: %d\n", state); return false; }

        // Heltec specific tuning
        state = sx1262.setDio2AsRfSwitch(true);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setDio2AsRfSwitch failed: %d\n", state); return false; }
        state = sx1262.setTCXO(1.6);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setTCXO failed: %d\n", state); return false; }
        state = sx1262.setRegulatorLDO();
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setRegulatorLDO failed: %d\n", state); return false; }
        state = sx1262.calibrateImage(targetFreq);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("calibrateImage failed: %d\n", state); return false; }

        state = sx1262.setPreambleLength(preambleLen);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setPreambleLength failed: %d\n", state); return false; }
        state = sx1262.setCRC(false);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setCRC failed: %d\n", state); return false; }
        state = sx1262.fixedPacketLengthMode(fixedPayloadLen);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("fixedPacketLengthMode failed: %d\n", state); return false; }
        state = sx1262.setSyncWord(syncWord, syncWordLen);
        if (state != RADIOLIB_ERR_NONE) { Serial.printf("setSyncWord failed: %d\n", state); return false; }

        sx1262.setDio1Action(isr);
        return true;
    }

    int16_t startReceive() { return sx1262.startReceive(); }

    int16_t readData(uint8_t* data, size_t& len) {
        len = sx1262.getPacketLength();
        return sx1262.readData(data, len);
    }

    float getInstantaneousRSSI() { return sx1262.getRSSI(false); }
    void setLed(bool state) { digitalWrite(HW_LED, state ? HIGH : LOW); }
}
#endif
