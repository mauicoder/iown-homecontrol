#ifndef RADIOMOCK_H
#define RADIOMOCK_H

#include <RadioLib.h> // For PhysicalLayer base class and RadioLib error codes
#include "protocols/PhysicalLayer/PhysicalLayer.h" // Ensure PhysicalLayer is defined
#include <vector>
#include <string>
#include <algorithm> // For std::copy

// Define missing RadioLib constants for testing environment if not pulled in
#ifndef RADIOLIB_IRQ_ALL
#define RADIOLIB_IRQ_ALL 0xFFFFFFFFU // A common full mask
#endif

// --- Mock PhysicalLayer for transmit/receive tests ---
class MockPhysicalLayer : public PhysicalLayer {
  using PhysicalLayer::setSyncWord; // Pull base class versions into scope
  using PhysicalLayer::startReceive; // Add this line to stop the "hiding" warning
public:
    // Mock specific variables to control behavior and capture values
    int16_t startTransmitResult = RADIOLIB_ERR_NONE;
    int16_t startReceiveResult = RADIOLIB_ERR_NONE;
    int16_t readDataResult = RADIOLIB_ERR_NONE;
    float actualFrequencySet = 0.0;
    std::vector<uint8_t> rxBufferInternal;
    size_t mockPacketLength = 0; // Length reported by getPacketLength

    // Constructor
    MockPhysicalLayer() : PhysicalLayer() {}

    // Override virtual functions from PhysicalLayer with mock implementations

    // Core operation mocks
    int16_t setFrequency(float freq) override {
        actualFrequencySet = freq;
        return RADIOLIB_ERR_NONE;
    }

    int16_t startTransmit(const uint8_t* data, size_t len, uint8_t addr = 0) override {
        (void)data; (void)len; (void)addr; // Suppress unused parameter warnings
        return startTransmitResult;
    }

    int16_t startReceive() override {
        return startReceiveResult;
    }

    size_t getPacketLength(bool update = true) override {
        (void)update; // Suppress unused parameter warning
        return mockPacketLength;
    }

    int16_t readData(uint8_t* data, size_t len) override {
        if (readDataResult != RADIOLIB_ERR_NONE) {
            return readDataResult;
        }
        if (len > rxBufferInternal.size()) {
            // Return an error if trying to read more than available in mock buffer
            return RADIOLIB_ERR_RX_TIMEOUT;
        }
        std::copy(rxBufferInternal.begin(), rxBufferInternal.begin() + len, data);
        return RADIOLIB_ERR_NONE;
    }

    // --- Pure Virtual Functions from PhysicalLayer.h that need to be implemented or mocked ---
    // These are the ones that were causing linker errors.
    // The ones not explicitly mocked above are provided with dummy implementations below.
    Module* getMod() override { return nullptr; }
    int16_t sleep() override { return RADIOLIB_ERR_NONE; }
    int16_t standby() override { return RADIOLIB_ERR_NONE; }
    int16_t standby(uint8_t mode) override { (void)mode; return RADIOLIB_ERR_NONE; }
    int16_t receive(uint8_t* data, size_t len, RadioLibTime_t timeout = 0) override { (void)data; (void)len; (void)timeout; return RADIOLIB_ERR_NONE; }
    int16_t finishTransmit() override { return RADIOLIB_ERR_NONE; }
    int16_t finishReceive() override { return RADIOLIB_ERR_NONE; }
    int16_t transmitDirect(uint32_t frf = 0) override { (void)frf; return RADIOLIB_ERR_NONE; }
    int16_t receiveDirect() override { return RADIOLIB_ERR_NONE; }
    int16_t setBitRate(float br) override { (void)br; return RADIOLIB_ERR_NONE; }
    int16_t setFrequencyDeviation(float freqDev) override { (void)freqDev; return RADIOLIB_ERR_NONE; }
    int16_t setDataShaping(uint8_t sh) override { (void)sh; return RADIOLIB_ERR_NONE; }
    int16_t setEncoding(uint8_t encoding) override { (void)encoding; return RADIOLIB_ERR_NONE; }
    int16_t invertIQ(bool enable) override { (void)enable; return RADIOLIB_ERR_NONE; }
    int16_t setOutputPower(int8_t power) override { (void)power; return RADIOLIB_ERR_NONE; }
    int16_t checkOutputPower(int8_t power, int8_t* clipped) override { (void)power; (void)clipped; return RADIOLIB_ERR_NONE; }
    int16_t setSyncWord(uint8_t* sync, size_t len) override { (void)sync; (void)len; return RADIOLIB_ERR_NONE; }
    int16_t setPreambleLength(size_t len) override { (void)len; return RADIOLIB_ERR_NONE; }
    int16_t setDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override { (void)dr; (void)modem; return RADIOLIB_ERR_NONE; }
    int16_t checkDataRate(DataRate_t dr, ModemType_t modem = RADIOLIB_MODEM_NONE) override { (void)dr; (void)modem; return RADIOLIB_ERR_NONE; }
    float getRSSI() override { return 0.0; }
    float getSNR() override { return 0.0; }
    RadioLibTime_t calculateTimeOnAir(ModemType_t modem, DataRate_t dr, PacketConfig_t pc, size_t len) override { (void)modem; (void)dr; (void)pc; (void)len; return 0; }
    RadioLibTime_t getTimeOnAir(size_t len) override { (void)len; return 0; }
    RadioLibTime_t calculateRxTimeout(RadioLibTime_t timeoutUs) override { (void)timeoutUs; return 0; }
    uint32_t getIrqFlags() override { return 0; }
    int16_t setIrqFlags(uint32_t irq) override { (void)irq; return RADIOLIB_ERR_NONE; }
    int16_t clearIrqFlags(uint32_t irq) override { (void)irq; return RADIOLIB_ERR_NONE; }
    int16_t startChannelScan() override { return RADIOLIB_ERR_NONE; }
    int16_t startChannelScan(const ChannelScanConfig_t &config) override { (void)config; return RADIOLIB_ERR_NONE; }
    int16_t getChannelScanResult() override { return RADIOLIB_ERR_NONE; }
    int16_t scanChannel() override { return RADIOLIB_ERR_NONE; }
    int16_t scanChannel(const ChannelScanConfig_t &config) override { (void)config; return RADIOLIB_ERR_NONE; }
    uint8_t randomByte() override { return 0; }
    int16_t setDIOMapping(uint32_t pin, uint32_t value) override { (void)pin; (void)value; return RADIOLIB_ERR_NONE; }
    void setPacketReceivedAction(void (*func)(void)) override { (void)func; }
    void clearPacketReceivedAction() override {}
    void setPacketSentAction(void (*func)(void)) override { (void)func; }
    void clearPacketSentAction() override {}
    void setChannelScanAction(void (*func)(void)) override { (void)func; }
    void clearChannelScanAction() override {}
    int16_t setModem(ModemType_t modem) override { (void)modem; return RADIOLIB_ERR_NONE; }
    int16_t getModem(ModemType_t* modem) override { (void)modem; return RADIOLIB_ERR_NONE; }
    int16_t stageMode(RadioModeType_t mode, RadioModeConfig_t* cfg) override { (void)mode; (void)cfg; return RADIOLIB_ERR_NONE; }
    int16_t launchMode() override { return RADIOLIB_ERR_NONE; }
    void setDirectAction(void (*func)(void)) override { (void)func; }
    void readBit(uint32_t pin) override { (void)pin; }

    // Non-virtual functions from original mock (if they are not part of PhysicalLayer interface)
    int16_t startReceive(uint32_t timeout, uint32_t channel) {
        (void)timeout; (void)channel;
        return RADIOLIB_ERR_NONE;
    }
    int16_t startTransmit(const std::string& str, uint32_t timeout, uint32_t channel) {
        (void)str; (void)timeout; (void)channel;
        return RADIOLIB_ERR_NONE;
    }
    float getRSSI(bool actual = false) { // This overloads the virtual getRSSI() with a different signature
        (void)actual;
        return 0.0;
    }
};

#endif // RADIOMOCK_H
