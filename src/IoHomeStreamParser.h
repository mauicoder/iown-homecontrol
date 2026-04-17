#pragma once

#include <Arduino.h>
#include "IoHome.h"
#include "IoHomeParser.h"

// --- PERSISTENT STATE MACHINE PARSER ---
class IoHomeStreamParser {
private:
    static const size_t BUFFER_SIZE = 1024;
    uint8_t buffer[BUFFER_SIZE];
    size_t length = 0;

    int frameBit = 0; // 0: wait for start, 1..8: data, 9: stop
    uint16_t currentData = 0;

    void consume(size_t count) {
        if (count >= length) {
            length = 0;
        } else {
            size_t remaining = length - count;
            memmove(buffer, buffer + count, remaining);
            length = remaining;
        }
    }

public:
    void pushBytes(const uint8_t* bytes, size_t len) {
        for (size_t i = 0; i < len; i++) {
            for (int b = 7; b >= 0; b--) {
                uint8_t bit = (bytes[i] >> b) & 0x01;
                if (frameBit == 0) {
                    if (bit == 0) { // Found Start bit (0)
                        frameBit = 1;
                        currentData = 0;
                    }
                } else if (frameBit >= 1 && frameBit <= 8) {
                    currentData |= (bit << (frameBit - 1)); // LSB first
                    frameBit++;
                } else if (frameBit == 9) {
                    if (bit == 1) { // Valid Stop bit (1)
                        if (length < BUFFER_SIZE) {
                            buffer[length++] = currentData;
                        }
                    }
                    frameBit = 0; // Reset for next byte
                }
            }
        }
    }

    bool extractNextFrame(IoHomeNode& ioNode, IoHomeFrame_t& outFrame, bool& outAuthStatus) {
        size_t searchIdx = 0;
        Serial.printf(">>> Searching rolling buffer (Current length: %zu bytes)\n", length);

        while (searchIdx + IOHOME_MIN_FRAME_LEN <= length) {
            uint8_t ctrlByte = buffer[searchIdx];

            // io-homecontrol MAC headers usually start with 0xFx (e.g., 0xF8, 0xF6, 0xF0)
            if ((ctrlByte & 0xF0) == 0xF0) {
                size_t declaredBodyLen = (ctrlByte & 0x1F) + 1;
                size_t expectedTotalLen = declaredBodyLen + IOHOME_FRAME_CRC_LEN;

                if (expectedTotalLen >= IOHOME_MIN_FRAME_LEN && expectedTotalLen <= 64) {
                    if (searchIdx + expectedTotalLen <= length) {
                        if (IoHomeNode::validateFrameCrc(&buffer[searchIdx], expectedTotalLen)) {
                            outAuthStatus = ioNode.parseFrame(&buffer[searchIdx], expectedTotalLen, outFrame);
                            consume(searchIdx + expectedTotalLen); // Remove parsed packet and preceding garbage
                            return true; // We found one!
                        }
                    } else {
                        Serial.printf("    [Parser] Found valid MAC header, but packet is split. Waiting for next chunk.\n");
                        break; // Valid-looking header, but packet is split. Wait for next chunk.
                    }
                }
            }
            searchIdx++;
        }
        if (searchIdx > 0) consume(searchIdx); // Drop evaluated garbage
        if (length > BUFFER_SIZE - 256) consume(length - 64); // Safe fallback
        return false;
    }

    void clear() {
        length = 0;
        frameBit = 0;
        currentData = 0;
    }
};
