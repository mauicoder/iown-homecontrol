#include "IoHome.h"
#include <RadioLib.h>
#include "TypeDef.h"
#include "protocols/PhysicalLayer/PhysicalLayer.h"
#include "debug_iohome.h" // For conditional Serial.printf
// Mock Serial object and PhysicalLayer base class definitions
// were moved to tests/test_IoHome.cpp to ensure vtable is emitted in the correct compilation unit for the test build.

IoHomeNode::IoHomeNode(PhysicalLayer* phy, const IoHomeChannel_t* channel_param)
  : _phyLayer(phy),
    _channel(channel_param),
    _sequence_counter(0) { // <--- ADD THIS

    // Initialize stack key with zeros or a default
    std::fill(std::begin(_stack_key), std::end(_stack_key), 0x00);
}

void IoHomeNode::begin(const IoHomeChannel_t* channel, NodeId source_id, NodeId dest_id, uint8_t* stack, uint8_t* system) {
    this->_channel = channel;
    this->_source_node_id = source_id;
    this->_destination_node_id = dest_id;

    if(stack) std::copy(stack, stack + 16, this->_stack_key);
    if(system) std::copy(system, system + 16, this->_system_key);

    this->setPhyProperties();
}

int16_t IoHomeNode::setPhyProperties() {

  int16_t state = 0; // RadioLib State
  int8_t  pwr   = 0; // RadioLib Tx Output Power in dBm

  pwr = 20;
  state = RADIOLIB_ERR_INVALID_OUTPUT_POWER;

  // RadioLib: Set Tx output power. Go from highest power and lower until we hit one supported by the module
  while(state == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {state = this->_phyLayer->setOutputPower(pwr--);}
  RADIOLIB_ASSERT(state);

  DataRate_t io_home_fskrate;
  io_home_fskrate.fsk.bitRate = 38400;
  io_home_fskrate.fsk.freqDev = 19200;

  state = this->_phyLayer->setDataRate(io_home_fskrate);
  RADIOLIB_ASSERT(state);
  state = this->_phyLayer->setDataShaping(RADIOLIB_SHAPING_NONE);
  RADIOLIB_ASSERT(state);
  state = this->_phyLayer->setEncoding(RADIOLIB_ENCODING_NRZ);
  RADIOLIB_ASSERT(state);

  uint8_t sync_word[IOHOME_SYNC_WORD_LEN] = { 0 };
  size_t pre_len = IOHOME_PREAMBLE_LEN / 8;
  sync_word[0] = (uint8_t)(IOHOME_SYNC_WORD >> 16);
  sync_word[1] = (uint8_t)(IOHOME_SYNC_WORD >> 8);
  sync_word[2] = (uint8_t) IOHOME_SYNC_WORD;

  state = this->_phyLayer->setSyncWord(sync_word, IOHOME_SYNC_WORD_LEN);
  RADIOLIB_ASSERT(state);

  state = this->_phyLayer->setPreambleLength(pre_len);
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

bool IoHomeNode::validateFrameCrc(const uint8_t* frame, size_t frameLength) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
  Serial.printf("[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: %u\n", frameLength);
#endif
  if (frameLength < IOHOME_FRAME_CRC_LEN) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::validateFrameCrc] Frame too short (len %u) to contain CRC (min %u).\n", frameLength, IOHOME_FRAME_CRC_LEN);
#endif
    return false; // Frame too short to even contain a CRC
  }

  // Calculate CRC over the data portion (excluding the 2 CRC bytes at the end)
  uint16_t calculatedCrc = IoHomeNode::crc16(frame, IOHOME_FRAME_CRC_POS(frameLength));

  // Extract the received CRC from the end of the frame
  uint16_t receivedCrc = IoHomeNode::ntoh<uint16_t>((uint8_t*)frame + IOHOME_FRAME_CRC_POS(frameLength));

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
  Serial.printf("[IoHomeNode::validateFrameCrc] Calculated CRC: 0x%04X, Received CRC: 0x%04X\n", calculatedCrc, receivedCrc);
#endif
  return calculatedCrc == receivedCrc;
}

std::vector<uint8_t> IoHomeNode::buildFrame(
  uint8_t ctrlByte0, uint8_t ctrlByte1,
  NodeId sourceMac, NodeId destMac,
  uint8_t commandId, const std::vector<uint8_t>& payload
) {
  // 1. Use your new Layer 3 constants for clarity
  const size_t headerLen = IOHOME_FRAME_HEADER_LEN;    // 8
  const size_t cmdLen    = IOHOME_COMMAND_ID_LEN;      // 1
  const size_t footerLen = IOHOME_SECURITY_FOOTER_LEN; // 8
  const size_t crcLen    = IOHOME_FRAME_CRC_LEN;       // 2

  // 2. Calculate Body Length
  size_t messageBodyLen = headerLen + cmdLen + payload.size() + footerLen;

  // 3. SAFETY CHECK: Catch the underflow before the vector allocation
  if (messageBodyLen < 1 || messageBodyLen > 255) {
      // If this triggers, your math or constants are broken
      throw std::runtime_error("Invalid messageBodyLen calculated");
  }

  // 4. Calculate Control Byte 0 Length Field
  // The spec says: FieldValue = (TotalBodyBytes - 1)
  uint8_t lenField = (uint8_t)((messageBodyLen - 1) & 0x1F);
  uint8_t finalCtrlByte0 = (ctrlByte0 & ~0x1F) | lenField;

  // 5. Allocate the frame
  // The length_error usually happens here if messageBodyLen is a massive underflowed number
  std::vector<uint8_t> frame(messageBodyLen + crcLen);
  size_t offset = 0;

  // --- Start Filling ---
  frame[offset++] = finalCtrlByte0;
  frame[offset++] = ctrlByte1;

  // MACs
  frame[offset++] = sourceMac.n0; frame[offset++] = sourceMac.n1; frame[offset++] = sourceMac.n2;
  frame[offset++] = destMac.n0;   frame[offset++] = destMac.n1;   frame[offset++] = destMac.n2;

  // Command
  frame[offset++] = commandId;

  // Payload
  if (!payload.empty()) {
    std::copy(payload.begin(), payload.end(), frame.begin() + offset);
    offset += payload.size();
  }

  // --- SECURITY FOOTER SECTION (Layer 3) ---

  // A: Insert the 2-byte Rolling Counter (Big Endian: High Byte, then Low Byte)
  frame[offset++] = (uint8_t)((this->_sequence_counter >> 8) & 0xFF); // High Byte
  frame[offset++] = (uint8_t)(this->_sequence_counter & 0xFF);        // Low Byte

  // B: Insert the 6-byte MAC (Signature) - Leaving as Zeros for now
  for(int i = 0; i < IOHOME_SECURITY_MAC_LEN; i++) {
    frame[offset++] = 0x00;
  }

  // CRC
  uint16_t calculatedCrc = IoHomeNode::crc16(frame.data(), messageBodyLen);
  // Note: Check if your hton is Big Endian; if so, CRC (LE) needs manual swap
  frame[offset++] = (uint8_t)(calculatedCrc & 0xFF);
  frame[offset++] = (uint8_t)((calculatedCrc >> 8) & 0xFF);

  // Increment internal state
  this->_sequence_counter++;

  return frame;
}

bool IoHomeNode::parseFrame(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame) {
    // Default to invalid and clear payload
    parsedFrame.isValid = false;
    parsedFrame.payload.clear();

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::parseFrame] Attempting to parse frame of length: %u\n", (unsigned int)frameLength);
    Serial.printf("[IoHomeNode::parseFrame] Raw frame: ");
    for (size_t i = 0; i < frameLength; ++i) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println("");
#endif

    // 1. Basic structural length check
    // Min frame: Header(8) + Cmd(1) + Security(8) + CRC(2) = 19 bytes
    if (frameLength < 19) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("[IoHomeNode::parseFrame] Frame too short for Layer 3.");
#endif
        return false;
    }

    // 2. Validate CRC
    if (!IoHomeNode::validateFrameCrc(frame, frameLength)) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("[IoHomeNode::parseFrame] CRC validation failed.");
#endif
        return false;
    }

    // 3. Extract Fixed Header
    size_t offset = 0;
    parsedFrame.ctrlByte0 = frame[offset++];
    parsedFrame.ctrlByte1 = frame[offset++];

    parsedFrame.sourceMac.n0 = frame[offset++];
    parsedFrame.sourceMac.n1 = frame[offset++];
    parsedFrame.sourceMac.n2 = frame[offset++];

    parsedFrame.destMac.n0 = frame[offset++];
    parsedFrame.destMac.n1 = frame[offset++];
    parsedFrame.destMac.n2 = frame[offset++];

    // 4. Extract Command ID
    parsedFrame.commandId = frame[offset++];

    // 5. Length Validation from CTRL0
    // IOHOME_MSG_LEN is typically ((frame[0] & 0x1F) + 1)
    size_t declared_msg_body_len = IOHOME_MSG_LEN(frame);

    if (declared_msg_body_len != (frameLength - IOHOME_FRAME_CRC_LEN)) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("[IoHomeNode::parseFrame] Inconsistent length bits.");
#endif
        return false;
    }

    // 6. Calculate Payload Length (Layer 3)
    // Overhead = Header(8) + Cmd(1) + SecurityFooter(8) = 17 bytes
    const size_t security_footer_len = 8;
    const size_t fixed_overhead = IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + security_footer_len;

    size_t expected_payload_len = 0;
    if (declared_msg_body_len >= fixed_overhead) {
        expected_payload_len = declared_msg_body_len - fixed_overhead;
    } else {
        return false; // Frame is too small to contain a security footer
    }

    // 7. Extract Payload
    if (expected_payload_len > 0) {
        parsedFrame.payload.resize(expected_payload_len);
        std::copy(frame + offset, frame + offset + expected_payload_len, parsedFrame.payload.begin());

        // IMPORTANT: Advance offset past the payload
        offset += expected_payload_len;

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.printf("[IoHomeNode::parseFrame] Parsed payload (len %u): ", (unsigned int)expected_payload_len);
        for (uint8_t b : parsedFrame.payload) Serial.printf("%02X ", b);
        Serial.println("");
#endif
    }

    // 8. Extract Security Footer (Counter + MAC)
    // Offset is now correctly pointing to the 2-byte counter
    uint16_t frameCounter = IoHomeNode::ntoh<uint16_t>((uint8_t*)frame + offset);
    (void)frameCounter; // Tells the Mac compiler "I know this might not be used here"
    offset += 2;

    uint8_t frameMac[6];
    std::copy(frame + offset, frame + offset + 6, frameMac);
    offset += 6;

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::parseFrame] Security: Counter=0x%04X, MAC=", frameCounter);
    for(int i = 0; i < 6; i++) Serial.printf("%02X ", frameMac[i]);
    Serial.println("");
#endif

    // Success
    parsedFrame.isValid = true;
    return true;
}

int16_t IoHomeNode::transmitFrame(const std::vector<uint8_t>& frame) {

    float freq = this->_channel->c0 + (this->_channel->c1 / 100.0);
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::transmitFrame] Setting frequency to %.2f MHz (Channel C0:%u, C1:%u)\n", freq, this->_channel->c0, this->_channel->c1);
#endif
    // Set frequency according to the current channel
    int16_t state = this->_phyLayer->setFrequency(freq);
    RADIOLIB_ASSERT(state);

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::transmitFrame] Attempting to transmit frame (len %u): ", frame.size());
    for (size_t i = 0; i < frame.size(); ++i) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println(""); // Use empty string for new line
#endif
    // Transmit the frame
    state = this->_phyLayer->startTransmit(frame.data(), frame.size());
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[IoHomeNode::transmitFrame] Transmission initiated successfully.");
    } else {
        Serial.printf("[IoHomeNode::transmitFrame] Transmission failed with error: %d\n", state);
    }
#endif
    return state;
}

int16_t IoHomeNode::receiveFrame(IoHomeFrame_t& receivedFrame) {
    float freq = this->_channel->c0 + (this->_channel->c1 / 100.0);
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::receiveFrame] Setting frequency to %.2f MHz (Channel C0:%u, C1:%u)\n", freq, this->_channel->c0, this->_channel->c1);
#endif
    // Set frequency according to the current channel
    int16_t state = this->_phyLayer->setFrequency(freq);
    RADIOLIB_ASSERT(state);

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.println("[IoHomeNode::receiveFrame] Starting receive mode.");
#endif
    // Start receiving
    state = this->_phyLayer->startReceive();
    RADIOLIB_ASSERT(state);

    // Wait for a packet
    size_t packetLength = this->_phyLayer->getPacketLength();
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::receiveFrame] Detected packet length: %u\n", packetLength);
#endif
    if (packetLength == 0) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("[IoHomeNode::receiveFrame] No packet received (length 0).");
#endif
        return RADIOLIB_ERR_RX_TIMEOUT; // Or some other appropriate error for no packet received
    }

    std::vector<uint8_t> rxBuffer(packetLength);
    state = this->_phyLayer->readData(rxBuffer.data(), packetLength);
    RADIOLIB_ASSERT(state);

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::receiveFrame] Read %u bytes from radio.\n", packetLength);
#endif

    // Parse the received frame
    if (!IoHomeNode::parseFrame(rxBuffer.data(), rxBuffer.size(), receivedFrame)) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("[IoHomeNode::receiveFrame] Frame parsing failed or CRC mismatch.");
#endif
        return RADIOLIB_ERR_CRC_MISMATCH; // Or a custom error for parsing failure
    }

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.println("[IoHomeNode::receiveFrame] Successfully received and parsed valid frame.");
#endif
    return RADIOLIB_ERR_NONE;
}
