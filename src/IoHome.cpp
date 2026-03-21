#include "IoHome.h"
#include <RadioLib.h>
#include "TypeDef.h"
#include "protocols/PhysicalLayer/PhysicalLayer.h"
#include <Arduino.h> // For Serial.print/printf

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

#ifdef DEBUG_IOHOME
  Serial.printf("[IoHomeNode::buildFrame] Payload size: %u, Message Body Length (excluding CRC): %u\n", payload.size(), messageBodyLen);
#endif
  
  // The length field in CTRLBYTE0 is (messageBodyLen - 1)
  // Mask out existing length bits and set new ones
  uint8_t finalCtrlByte0 = (ctrlByte0 & ~0x1F) | ((messageBodyLen - 1) & 0x1F); // Ensure length field fits 5 bits

#ifdef DEBUG_IOHOME
  Serial.printf("[IoHomeNode::buildFrame] Original Ctrl0: 0x%02X, Final Ctrl0 (with len): 0x%02X\n", ctrlByte0, finalCtrlByte0);
  Serial.printf("[IoHomeNode::buildFrame] Ctrl1: 0x%02X\n", ctrlByte1);
  Serial.printf("[IoHomeNode::buildFrame] Source MAC: %02X:%02X:%02X, Dest MAC: %02X:%02X:%02X\n",
                sourceMac.n0, sourceMac.n1, sourceMac.n2, destMac.n0, destMac.n1, destMac.n2);
  Serial.printf("[IoHomeNode::buildFrame] Command ID: 0x%02X\n", commandId);
#endif

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

#ifdef DEBUG_IOHOME
  Serial.printf("[IoHomeNode::buildFrame] Calculated CRC for message body: 0x%04X\n", calculatedCrc);
#endif

  // Append CRC to the frame (resize the vector first)
  frame.resize(messageBodyLen + IOHOME_FRAME_CRC_LEN);
  // Use hton to place the 16-bit CRC into the frame at the correct offset
  IoHomeNode::hton<uint16_t>(frame.data() + offset, calculatedCrc);

#ifdef DEBUG_IOHOME
  Serial.printf("[IoHomeNode::buildFrame] Built frame (len %u): ", frame.size());
  for (size_t i = 0; i < frame.size(); ++i) {
    Serial.printf("%02X ", frame[i]);
  }
  Serial.println();
#endif

  return frame;
}

bool IoHomeNode::parseFrame(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame) {
    // Default to invalid and clear payload
    parsedFrame.isValid = false;
    parsedFrame.payload.clear();

#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::parseFrame] Attempting to parse frame of length: %u\n", frameLength);
    Serial.printf("[IoHomeNode::parseFrame] Raw frame: ");
    for (size_t i = 0; i < frameLength; ++i) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println();
#endif

    // 1. Basic length check: must be at least header + cmd_id + CRC
    if (frameLength < (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + IOHOME_FRAME_CRC_LEN)) {
#ifdef DEBUG_IOHOME
        Serial.printf("[IoHomeNode::parseFrame] Frame too short (len %u) to be valid (min %u).\n", frameLength, (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + IOHOME_FRAME_CRC_LEN));
#endif
        return false;
    }

    // 2. Validate CRC
    if (!IoHomeNode::validateFrameCrc(frame, frameLength)) {
#ifdef DEBUG_IOHOME
        Serial.println("[IoHomeNode::parseFrame] CRC validation failed.");
#endif
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
    
#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::parseFrame] Declared message body length from CTRL0 (0x%02X): %u\n", parsedFrame.ctrlByte0, declared_msg_body_len);
    Serial.printf("[IoHomeNode::parseFrame] Actual message body length (excluding CRC): %u\n", (frameLength - IOHOME_FRAME_CRC_LEN));
#endif

    // The actual data length covered by CRC should match the declared length
    if (declared_msg_body_len != (frameLength - IOHOME_FRAME_CRC_LEN)) {
#ifdef DEBUG_IOHOME
        Serial.println("[IoHomeNode::parseFrame] Inconsistent declared length vs. actual frame length.");
#endif
        return false; // Inconsistent declared length
    }

    // Calculate payload length
    size_t expected_payload_len = declared_msg_body_len - (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN);

#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::parseFrame] Expected payload length: %u\n", expected_payload_len);
#endif

    // Copy payload
    if (expected_payload_len > 0) {
        // Ensure there's enough data remaining in the frame for the declared payload
        if (offset + expected_payload_len > frameLength - IOHOME_FRAME_CRC_LEN) {
#ifdef DEBUG_IOHOME
            Serial.printf("[IoHomeNode::parseFrame] Payload length (%u) exceeds available data (%u) before CRC.\n", expected_payload_len, (frameLength - IOHOME_FRAME_CRC_LEN - offset));
#endif
            return false; // Payload length exceeds available data before CRC
        }
        parsedFrame.payload.resize(expected_payload_len);
        std::copy(frame + offset, frame + offset + expected_payload_len, parsedFrame.payload.begin());
#ifdef DEBUG_IOHOME
        Serial.printf("[IoHomeNode::parseFrame] Parsed payload (len %u): ", expected_payload_len);
        for (size_t i = 0; i < parsedFrame.payload.size(); ++i) {
            Serial.printf("%02X ", parsedFrame.payload[i]);
        }
        Serial.println();
#endif
    } else {
#ifdef DEBUG_IOHOME
        Serial.println("[IoHomeNode::parseFrame] No payload detected.");
#endif
    }

    parsedFrame.isValid = true; // Mark as valid after successful parsing
#ifdef DEBUG_IOHOME
    Serial.println("[IoHomeNode::parseFrame] Successfully parsed frame.");
    Serial.printf("[IoHomeNode::parseFrame] Parsed data: Ctrl0=0x%02X, Ctrl1=0x%02X, SrcMAC=%02X:%02X:%02X, DestMAC=%02X:%02X:%02X, CmdID=0x%02X\n",
                  parsedFrame.ctrlByte0, parsedFrame.ctrlByte1,
                  parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                  parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2,
                  parsedFrame.commandId);
#endif
    return true; // Parsing successful and CRC valid
}

int16_t IoHomeNode::transmitFrame(const std::vector<uint8_t>& frame) {
    float freq = this->channel->c0 + (this->channel->c1 / 100.0);
#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::transmitFrame] Setting frequency to %.2f MHz (Channel C0:%u, C1:%u)\n", freq, this->channel->c0, this->channel->c1);
#endif
    // Set frequency according to the current channel
    int16_t state = this->phyLayer->setFrequency(freq);
    RADIOLIB_ASSERT(state);

#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::transmitFrame] Attempting to transmit frame (len %u): ", frame.size());
    for (size_t i = 0; i < frame.size(); ++i) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println();
#endif
    // Transmit the frame
    state = this->phyLayer->startTransmit(frame.data(), frame.size());
#ifdef DEBUG_IOHOME
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[IoHomeNode::transmitFrame] Transmission initiated successfully.");
    } else {
        Serial.printf("[IoHomeNode::transmitFrame] Transmission failed with error: %d\n", state);
    }
#endif
    return state;
}

int16_t IoHomeNode::receiveFrame(IoHomeFrame_t& receivedFrame) {
    float freq = this->channel->c0 + (this->channel->c1 / 100.0);
#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::receiveFrame] Setting frequency to %.2f MHz (Channel C0:%u, C1:%u)\n", freq, this->channel->c0, this->channel->c1);
#endif
    // Set frequency according to the current channel
    int16_t state = this->phyLayer->setFrequency(freq);
    RADIOLIB_ASSERT(state);

#ifdef DEBUG_IOHOME
    Serial.println("[IoHomeNode::receiveFrame] Starting receive mode.");
#endif
    // Start receiving
    state = this->phyLayer->startReceive();
    RADIOLIB_ASSERT(state);

    // Wait for a packet
    size_t packetLength = this->phyLayer->getPacketLength();
#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::receiveFrame] Detected packet length: %u\n", packetLength);
#endif
    if (packetLength == 0) {
#ifdef DEBUG_IOHOME
        Serial.println("[IoHomeNode::receiveFrame] No packet received (length 0).");
#endif
        return RADIOLIB_ERR_RX_TIMEOUT; // Or some other appropriate error for no packet received
    }

    std::vector<uint8_t> rxBuffer(packetLength);
    state = this->phyLayer->readData(rxBuffer.data(), packetLength);
    RADIOLIB_ASSERT(state);

#ifdef DEBUG_IOHOME
    Serial.printf("[IoHomeNode::receiveFrame] Read %u bytes from radio.\n", packetLength);
#endif

    // Parse the received frame
    if (!IoHomeNode::parseFrame(rxBuffer.data(), rxBuffer.size(), receivedFrame)) {
#ifdef DEBUG_IOHOME
        Serial.println("[IoHomeNode::receiveFrame] Frame parsing failed or CRC mismatch.");
#endif
        return RADIOLIB_ERR_CRC_MISMATCH; // Or a custom error for parsing failure
    }

#ifdef DEBUG_IOHOME
    Serial.println("[IoHomeNode::receiveFrame] Successfully received and parsed valid frame.");
#endif
    return RADIOLIB_ERR_NONE;
}

