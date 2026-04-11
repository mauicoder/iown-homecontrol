#pragma once

#include <cstdint>
#include <cstddef>
#include "TypeDef.h"
#include "IoHome.h"

class IoHomeCrypto {
public:
    // Decrypt the raw 16-byte key hidden inside a 0x30 command payload
    static bool decryptTransferKey(const IoHomeFrame_t& parsedFrame, uint8_t* outKey);
    // Computes the dynamic 16-byte IV block and performs AES encryption to generate MAC
    static void generateMac(const IoHomeFrame_t& parsedFrame, const uint8_t* frameData, uint16_t sequenceCounter, const uint8_t* key, uint8_t* outMac);
    // Validates the frame MAC against a provided active key
    static bool verifyMac(const IoHomeFrame_t& parsedFrame, const uint8_t* frameData, uint16_t rxCounter, const uint8_t* rxMac, size_t macLen, const uint8_t* activeKey);
};
