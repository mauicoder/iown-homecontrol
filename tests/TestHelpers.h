#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "IoHome.h"
#include <cstddef> // For size_t

// Centralized "Layer 3" Frame Length Logic
inline size_t calculateExpectedFrameLen(size_t payloadSize) {
    // Header: CTRL0(1) + CTRL1(1) + SRC(3) + DEST(3) = 8
    // Command: 1
    // Security Footer: Counter(2) + MAC(6) = 8
    // Checksum: CRC(2)

    size_t header = IOHOME_FRAME_HEADER_LEN;
    size_t cmdId  = IOHOME_COMMAND_ID_LEN;
    size_t footer = 8; // 2 byte counter + 6 byte MAC
    size_t crc    = IOHOME_FRAME_CRC_LEN;

    return header + cmdId + payloadSize + footer + crc;
}

// In tests/TestHelpers.h

struct IoHomeTestFixture {
    // 1. Create the dependencies for the node
    MockPhysicalLayer mockPhy;
    IoHomeChannel_t dummyChannel = { 868, 0 }; // Just a placeholder, your tests can set specific values if needed

    // 2. The node instance
    IoHomeNode node;

    // Constants for tests
    uint8_t ctrl0_base = IOHOME_CTRLBYTE0_MODE_TWOWAY | IOHOME_CTRLBYTE0_ORDER_0;
    uint8_t ctrl1 = 0xAA;
    NodeId src_mac = {0x11, 0x22, 0x33};
    NodeId dest_mac = {0x44, 0x55, 0x66};
    uint8_t cmd_id = 0x01;

    // 3. The constructor passes the addresses of the mock objects to the node
    IoHomeTestFixture() : node(&mockPhy, &dummyChannel) {
        // You can set the MAC here if your node has a setMac method,
        // or just rely on the default for now.
    }
};

#endif
