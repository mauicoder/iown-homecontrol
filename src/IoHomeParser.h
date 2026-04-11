#pragma once

#include "IoHome.h"
#include <cstdint>
#include <cstddef>
#include "TypeDef.h"

class IoHomeParser {
public:
    // Calculate standard io-homecontrol CRC16-KERMIT
    static uint16_t crc16(const uint8_t* data, size_t length);
    static bool validateFrameCrc(const uint8_t* frame, size_t frameLength);
    // Decode structural header and dynamic payload sizing without performing AES checks
    static bool decodeHeader(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame, size_t& outActualMacLen, uint16_t& outRxCounter, uint8_t* outRxMac);
};
