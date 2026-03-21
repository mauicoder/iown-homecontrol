#include "IoHome.h"
#include <RadioLib.h>
#include "TypeDef.h"
#include "protocols/PhysicalLayer/PhysicalLayer.h"

IoHomeNode::IoHomeNode(PhysicalLayer* phy, const IoHomeChannel_t* channel_param)
  : phyLayer(phy), channel(channel_param) {
}

void IoHomeNode::begin(const IoHomeChannel_t* channel, NodeId source_node_id, NodeId destination_node_id, uint8_t* stack_key, uint8_t* system_key) {}

int16_t IoHomeNode::setPhyProperties() {

  int16_t state = 0; // RadioLib State
  int8_t  pwr   = 0; // RadioLib Tx Output Power in dBm

  pwr = 20;
  state = RADIOLIB_ERR_INVALID_OUTPUT_POWER;

  // RadioLib: Set Tx output power. Go from highest power and lower until we hit one supported by the module
  while(state == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {state = this->phyLayer->setOutputPower(pwr--);}
  RADIOLIB_ASSERT(state);

  DataRate_t io_home_fskrate;
  io_home_fskrate.fsk.bitRate = 38400;
  io_home_fskrate.fsk.freqDev = 19200;

  state = this->phyLayer->setDataRate(io_home_fskrate);
  RADIOLIB_ASSERT(state);
  state = this->phyLayer->setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);
  state = this->phyLayer->setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  uint8_t sync_word[IOHOME_SYNC_WORD_LEN] = { 0 };
  size_t pre_len = IOHOME_PREAMBLE_LEN / 8;
  sync_word[0] = (uint8_t)(IOHOME_SYNC_WORD >> 16);
  sync_word[1] = (uint8_t)(IOHOME_SYNC_WORD >> 8);
  sync_word[2] = (uint8_t) IOHOME_SYNC_WORD;

  state = this->phyLayer->setSyncWord(sync_word, IOHOME_SYNC_WORD_LEN);
  RADIOLIB_ASSERT(state);

  state = this->phyLayer->setPreambleLength(pre_len);
  RADIOLIB_ASSERT(state);
  return(state);
} // set the physical layer configuration

uint16_t IoHomeNode::crc16(const uint8_t* data, size_t length) {
  // CRC-16/CCITT (KERMIT) - Polynomial: 0x8408 (0x1021 bit-reversed), Init: 0x0000
  uint16_t crc = 0x0000;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0x8408;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

