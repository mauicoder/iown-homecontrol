#include "IoHome.h"

struct IoHomeTestFixture {
    uint8_t ctrl0_base = IOHOME_CTRLBYTE0_MODE_TWOWAY | IOHOME_CTRLBYTE0_ORDER_0;
    uint8_t ctrl1 = 0xAA;
    NodeId src_mac = {0x11, 0x22, 0x33};
    NodeId dest_mac = {0x44, 0x55, 0x66};
    uint8_t cmd_id = 0x01;
};
