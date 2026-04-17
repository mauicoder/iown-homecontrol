#include "RadioMock.h"

// --- BASE CLASS CONSTRUCTOR ---
PhysicalLayer::PhysicalLayer() : freqStep(0.0), maxPacketLength(0) {}

// --- THE WALL OF STUBS ---
// These satisfy the vtable for the PhysicalLayer base class on your Mac.
int16_t PhysicalLayer::transmit(const uint8_t*, size_t, uint8_t) { return 0; }
int16_t PhysicalLayer::sleep() { return 0; }
int16_t PhysicalLayer::standby() { return 0; }
int16_t PhysicalLayer::standby(uint8_t) { return 0; }
int16_t PhysicalLayer::startReceive() { return 0; }
int16_t PhysicalLayer::startReceive(uint32_t, uint32_t, uint32_t, size_t) { return 0; }
int16_t PhysicalLayer::receive(uint8_t*, size_t, RadioLibTime_t) { return 0; }
int16_t PhysicalLayer::startTransmit(const uint8_t*, size_t, uint8_t) { return 0; }
int16_t PhysicalLayer::finishTransmit() { return 0; }
int16_t PhysicalLayer::finishReceive() { return 0; }
int16_t PhysicalLayer::readData(uint8_t*, size_t) { return 0; }
int16_t PhysicalLayer::transmitDirect(uint32_t) { return 0; }
int16_t PhysicalLayer::receiveDirect() { return 0; }
int16_t PhysicalLayer::setFrequency(float) { return 0; }
int16_t PhysicalLayer::setBitRate(float) { return 0; }
int16_t PhysicalLayer::setFrequencyDeviation(float) { return 0; }
int16_t PhysicalLayer::setDataShaping(uint8_t) { return 0; }
int16_t PhysicalLayer::setEncoding(uint8_t) { return 0; }
int16_t PhysicalLayer::invertIQ(bool) { return 0; }
int16_t PhysicalLayer::setOutputPower(int8_t) { return 0; }
int16_t PhysicalLayer::checkOutputPower(int8_t, int8_t*) { return 0; }
int16_t PhysicalLayer::setSyncWord(uint8_t*, size_t) { return 0; }
int16_t PhysicalLayer::setPreambleLength(size_t) { return 0; }
int16_t PhysicalLayer::setDataRate(DataRate_t, ModemType_t) { return 0; }
int16_t PhysicalLayer::checkDataRate(DataRate_t, ModemType_t) { return 0; }
RadioLibTime_t PhysicalLayer::calculateTimeOnAir(ModemType_t, DataRate_t, PacketConfig_t, size_t) { return 0; }
RadioLibTime_t PhysicalLayer::getTimeOnAir(size_t) { return 0; }
RadioLibTime_t PhysicalLayer::calculateRxTimeout(RadioLibTime_t) { return 0; }
uint32_t PhysicalLayer::getIrqFlags() { return 0; }
int16_t PhysicalLayer::setIrqFlags(uint32_t) { return 0; }
int16_t PhysicalLayer::clearIrqFlags(uint32_t) { return 0; }
int16_t PhysicalLayer::startChannelScan() { return 0; }
int16_t PhysicalLayer::startChannelScan(const ChannelScanConfig_t&) { return 0; }
int16_t PhysicalLayer::getChannelScanResult() { return 0; }
int16_t PhysicalLayer::scanChannel() { return 0; }
int16_t PhysicalLayer::scanChannel(const ChannelScanConfig_t&) { return 0; }
uint8_t PhysicalLayer::randomByte() { return 0; }
int16_t PhysicalLayer::setDIOMapping(uint32_t, uint32_t) { return 0; }
void PhysicalLayer::setPacketReceivedAction(void (*)(void)) {}
void PhysicalLayer::clearPacketReceivedAction() {}
void PhysicalLayer::setPacketSentAction(void (*)(void)) {}
void PhysicalLayer::clearPacketSentAction() {}
void PhysicalLayer::setChannelScanAction(void (*)(void)) {}
void PhysicalLayer::clearChannelScanAction() {}
int16_t PhysicalLayer::setModem(ModemType_t) { return 0; }
int16_t PhysicalLayer::getModem(ModemType_t*) { return 0; }
int16_t PhysicalLayer::stageMode(RadioModeType_t, RadioModeConfig_t*) { return 0; }
int16_t PhysicalLayer::launchMode() { return 0; }
void PhysicalLayer::setDirectAction(void (*)(void)) {}
void PhysicalLayer::readBit(uint32_t) {}
float PhysicalLayer::getRSSI() { return 0.0; }
float PhysicalLayer::getSNR() { return 0.0; }
size_t PhysicalLayer::getPacketLength(bool) { return 0; }
