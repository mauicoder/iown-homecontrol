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

std::vector<uint8_t> IoHomeNode::buildFrame(
  uint8_t ctrlByte0,
  uint8_t ctrlByte1,
  NodeId sourceMac,
  NodeId destMac,
  uint8_t commandId,
  const std::vector<uint8_t>& payload
) {
  // Calculate the total length of the message body (data to be CRC'd)
  // This includes CTRL0, CTRL1, Source MAC, Dest MAC, Command ID, and Payload
  size_t messageBodyLen = IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + payload.size();
  
  // The length field in CTRLBYTE0 is (messageBodyLen - 1)
  // Mask out existing length bits and set new ones
  uint8_t finalCtrlByte0 = (ctrlByte0 & ~0x1F) | (messageBodyLen - 1);

  // Initialize frame with space for message body + CRC
  std::vector<uint8_t> frame(messageBodyLen);

  size_t offset = 0;
  frame[offset++] = finalCtrlByte0;
  frame[offset++] = ctrlByte1;

  frame[offset++] = sourceMac.n0;
  frame[offset++] = sourceMac.n1;
  frame[offset++] = sourceMac.n2;

  frame[offset++] = destMac.n0;
  frame[offset++] = destMac.n1;
  frame[offset++] = destMac.n2;

  frame[offset++] = commandId;

  // Copy payload if present
  if (!payload.empty()) {
    std::copy(payload.begin(), payload.end(), frame.begin() + offset);
    offset += payload.size();
  }

  // Calculate CRC over the assembled message body
  uint16_t calculatedCrc = IoHomeNode::crc16(frame.data(), messageBodyLen);

  // Append CRC to the frame (resize the vector first)
  frame.resize(messageBodyLen + IOHOME_FRAME_CRC_LEN);
  // Use hton to place the 16-bit CRC into the frame at the correct offset
  IoHomeNode::hton<uint16_t>(frame.data() + offset, calculatedCrc);

  return frame;
}

