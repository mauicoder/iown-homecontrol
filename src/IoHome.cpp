#include "IoHome.h"
#include <RadioLib.h>
#include "TypeDef.h"
#include "protocols/PhysicalLayer/PhysicalLayer.h"

IoHomeNode::IoHomeNode(PhysicalLayer* phy, const IoHomeChannel_t* channel_param)
  : phyLayer(phy), channel(channel_param) {
}

void IoHomeNode::begin(const IoHomeChannel_t* channel, NodeId source_node_id, NodeId destination_node_id, uint8_t* stack_key, uint8_t* system_key) {
  (void)channel;
  (void)source_node_id;
  (void)destination_node_id;
  (void)stack_key;
  (void)system_key;
}

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

bool IoHomeNode::parseFrame(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame) {
    // Default to invalid and clear payload
    parsedFrame.isValid = false;
    parsedFrame.payload.clear();

    // 1. Basic length check: must be at least header + cmd_id + CRC
    if (frameLength < (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + IOHOME_FRAME_CRC_LEN)) {
        return false;
    }

    // 2. Validate CRC
    if (!IoHomeNode::validateFrameCrc(frame, frameLength)) {
        return false;
    }

    // Now that CRC is validated, proceed with parsing
    size_t offset = 0;
    parsedFrame.ctrlByte0 = frame[offset++];
    parsedFrame.ctrlByte1 = frame[offset++];

    parsedFrame.sourceMac.n0 = frame[offset++];
    parsedFrame.sourceMac.n1 = frame[offset++];
    parsedFrame.sourceMac.n2 = frame[offset++];

    parsedFrame.destMac.n0 = frame[offset++];
    parsedFrame.destMac.n1 = frame[offset++];
    parsedFrame.destMac.n2 = frame[offset++];

    parsedFrame.commandId = frame[offset++];

    // Determine expected message body length from CTRLBYTE0 (excluding CRC)
    size_t declared_msg_body_len = IOHOME_MSG_LEN(frame); // This is CTRL0 + ... + Payload
    
    // The actual data length covered by CRC should match the declared length
    if (declared_msg_body_len != (frameLength - IOHOME_FRAME_CRC_LEN)) {
        return false; // Inconsistent declared length
    }

    // Calculate payload length
    size_t expected_payload_len = declared_msg_body_len - (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN);

    // Copy payload
    if (expected_payload_len > 0) {
        // Ensure there's enough data remaining in the frame for the declared payload
        if (offset + expected_payload_len > frameLength - IOHOME_FRAME_CRC_LEN) {
            return false; // Payload length exceeds available data before CRC
        }
        parsedFrame.payload.resize(expected_payload_len);
        std::copy(frame + offset, frame + offset + expected_payload_len, parsedFrame.payload.begin());
    }

    parsedFrame.isValid = true; // Mark as valid after successful parsing
    return true; // Parsing successful and CRC valid
}

int16_t IoHomeNode::transmitFrame(const std::vector<uint8_t>& frame) {
    // Set frequency according to the current channel
    int16_t state = this->phyLayer->setFrequency(this->channel->c0 + (this->channel->c1 / 100.0));
    RADIOLIB_ASSERT(state);

    // Transmit the frame
    state = this->phyLayer->startTransmit(frame.data(), frame.size());
    return state;
}

int16_t IoHomeNode::receiveFrame(IoHomeFrame_t& receivedFrame) {
    // Set frequency according to the current channel
    int16_t state = this->phyLayer->setFrequency(this->channel->c0 + (this->channel->c1 / 100.0));
    RADIOLIB_ASSERT(state);

    // Start receiving
    state = this->phyLayer->startReceive();
    RADIOLIB_ASSERT(state);

    // Wait for a packet
    size_t packetLength = this->phyLayer->getPacketLength();
    if (packetLength == 0) {
        return RADIOLIB_ERR_RX_TIMEOUT; // Or some other appropriate error for no packet received
    }

    std::vector<uint8_t> rxBuffer(packetLength);
    state = this->phyLayer->readData(rxBuffer.data(), packetLength);
    RADIOLIB_ASSERT(state);

    // Parse the received frame
    if (!IoHomeNode::parseFrame(rxBuffer.data(), rxBuffer.size(), receivedFrame)) {
        return RADIOLIB_ERR_CRC_MISMATCH; // Or a custom error for parsing failure
    }

    return RADIOLIB_ERR_NONE;
}

