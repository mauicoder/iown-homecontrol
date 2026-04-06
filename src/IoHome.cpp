#include "IoHome.h"
#include <RadioLib.h>
#include <mbedtls/aes.h>
#include <stdexcept>
#include "TypeDef.h"
#include "protocols/PhysicalLayer/PhysicalLayer.h"
// Mock Serial object and PhysicalLayer base class definitions
// were moved to tests/test_IoHome.cpp to ensure vtable is emitted in the correct compilation unit for the test build.

IoHomeNode::IoHomeNode(PhysicalLayer* phy, const IoHomeChannel_t* channel_param)
  : _phyLayer(phy),
    _channel(channel_param),
    _source_node_id({0, 0, 0}),      // Initialize NodeIDs to zero
    _destination_node_id({0, 0, 0}),
    _sequence_counter(0) {           // Ensure counter starts at 0

    // Initialize security keys with zeros to avoid garbage memory
    std::fill(std::begin(_stack_key), std::end(_stack_key), 0x00);
    std::fill(std::begin(_system_key), std::end(_system_key), 0x00);
}

int16_t IoHomeNode::begin(const IoHomeChannel_t* channel,
                         NodeId source_node_id,
                         NodeId destination_node_id,
                         uint8_t* stack_key,
                         uint8_t* system_key) {
    // 1. Core Data Setup (Keep this - it's protocol logic)
    this->_channel = channel;
    this->_source_node_id = source_node_id;
    this->_destination_node_id = destination_node_id;

    if (stack_key != nullptr) {
        std::copy(stack_key, stack_key + 16, this->_stack_key);
    }
    if (system_key != nullptr) {
        std::copy(system_key, system_key + 16, this->_system_key);
    }

    this->_sequence_counter = 0;

    // 2. Hardware Safety Check
    if (this->_phyLayer == nullptr || this->_channel == nullptr) {
        return RADIOLIB_ERR_CHIP_NOT_FOUND;
    }

    // --- STRIPPED HARDWARE CALLS ---
    // We NO LONGER call setFrequency, setBitRate, or setSyncWord here.
    // We assume the HAL (main.cpp) has already prepared the radio.

    return RADIOLIB_ERR_NONE;
}

uint16_t IoHomeNode::crc16(const uint8_t* data, size_t length) {
  // CRC-16/CCITT (KERMIT) - Polynomial: 0x8408 (0x1021 bit-reversed), Init: 0x0000
  uint16_t crc = 0x0000;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ IOHOME_CRC_POLY;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// Helper function to reverse the bit order of a byte.
// This is necessary to match the on-air LSB-first transmission format.
static uint8_t reverse_bits(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void IoHomeNode::deWhiten(uint8_t* data, size_t length) {
    // This is a software implementation of the standard PN9 de-whitening process.
    // The transmitter must be using this algorithm for our SX1262 to be compatible.
    // The key is that the protocol documentation states "bits of each byte are swapped".

    uint16_t lfsr = IOHOME_PN9_LFSR_INIT; // Standard PN9 initial state

    for (size_t i = 0; i < length; i++) {
        // 1. Reverse the bits of the received byte to match the on-air LSB-first sequence.
        uint8_t reversed_whitened_byte = reverse_bits(data[i]);

        uint8_t dewhitened_byte = 0;
        for (int j = 0; j < 8; j++) {
            // 2. Generate the next bit of the PN9 whitening sequence.
            uint8_t lfsr_out = (lfsr >> 8) & 0x01;

            // 3. XOR the bit from the reversed byte (processing MSB to LSB, which is on-air LSB to MSB)
            uint8_t in_bit = (reversed_whitened_byte >> (7 - j)) & 0x01;
            dewhitened_byte |= ((in_bit ^ lfsr_out) << (7 - j));

            // 4. Update the LFSR state using the standard PN9 polynomial (x^9 + x^5 + 1).
            // The new bit is an XOR of the 9th and 5th bits (indexed 8 and 4).
            uint16_t new_bit = ((lfsr >> 8) ^ (lfsr >> 4)) & 0x01;
            lfsr = ((lfsr << 1) | new_bit) & 0x1FF;
        }
        // 5. Reverse the de-whitened byte back to the standard in-memory order.
        data[i] = reverse_bits(dewhitened_byte);
    }
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
  // io-homecontrol CRC is transmitted Little-Endian
  uint16_t receivedCrc = frame[IOHOME_FRAME_CRC_POS(frameLength)] | (frame[IOHOME_FRAME_CRC_POS(frameLength) + 1] << 8);

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

// --- LAYER 3: AES-128 MAC (Message Authentication Code) ---
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  // 1. Load the 16-byte Stack Key
  mbedtls_aes_setkey_enc(&aes, this->_stack_key, 128);

  // 2. Prepare the Input Block (16 bytes)
  // Per io-homecontrol: MAC = AES128(Header[8] | Command[1] | Counter[2] | Padding[5])
  uint8_t input_block[16] = {0};
  std::copy(frame.begin(), frame.begin() + 8, input_block);     // Header
  input_block[8] = commandId;                                   // Command
  input_block[9] = (uint8_t)(this->_sequence_counter >> 8);     // Counter High
  input_block[10] = (uint8_t)(this->_sequence_counter & 0xFF);  // Counter Low
  // Bytes 11-15 remain 0x00 (standard padding)

  // 3. Encrypt the block
  uint8_t output_block[16] = {0};
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input_block, output_block);

  // 4. Extract 6 bytes and insert into frame
  for (int i = 0; i < IOHOME_SECURITY_MAC_LEN; i++) {
      frame[offset++] = output_block[i];
  }

  mbedtls_aes_free(&aes);

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
    Serial.printf("[IoHomeNode::parseFrame] --- Starting parse for frame of length: %u ---\n", (unsigned int)frameLength);
    // Log raw bytes for debugging
    Serial.print("[IoHomeNode::parseFrame] Raw bytes: ");
    for(size_t i=0; i<frameLength; i++) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println();
#endif

    // 1. Basic structural length check (Header + CmdID + Counter + CRC)
    if (frameLength < (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + IOHOME_SECURITY_COUNTER_LEN + IOHOME_FRAME_CRC_LEN)) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.printf("[IoHomeNode::parseFrame] Frame too short (len %u) for minimum header+cmd+counter+crc (%u bytes).\n", frameLength, (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + IOHOME_SECURITY_COUNTER_LEN + IOHOME_FRAME_CRC_LEN));
#endif
        return false;
    }

    // 2. Validate CRC
    if (!validateFrameCrc(frame, frameLength)) {
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

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0x%02X (Binary: %s), Ctrl1=0x%02X (Binary: %s)\n",
                  parsedFrame.ctrlByte0, String(parsedFrame.ctrlByte0, BIN).c_str(),
                  parsedFrame.ctrlByte1, String(parsedFrame.ctrlByte1, BIN).c_str());
    Serial.printf("[IoHomeNode::parseFrame] Source MAC: %02X%02X%02X, Dest MAC: %02X%02X%02X\n",
                  parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                  parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2);
    Serial.printf("[IoHomeNode::parseFrame] Command ID: 0x%02X\n", parsedFrame.commandId);
#endif

    // 5. Length Validation from CTRL0
    // The lower 5 bits of Ctrl0 represent TotalBodyBytes - 1
    size_t total_body_bytes = (parsedFrame.ctrlByte0 & 0x1F) + 1;
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::parseFrame] Declared total body length from Ctrl0: %u bytes\n", total_body_bytes);
#endif

    // Determine actual security footer length based on observed frame length and declared payload length
    size_t actual_security_mac_len = IOHOME_SECURITY_MAC_LEN; // Default to 6
    size_t actual_security_footer_len = IOHOME_SECURITY_COUNTER_LEN + actual_security_mac_len;

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::parseFrame] Assuming security footer length: %u (Counter: %u, MAC: %u)\n",
                  (unsigned int)actual_security_footer_len, (unsigned int)IOHOME_SECURITY_COUNTER_LEN, (unsigned int)actual_security_mac_len);
#endif

    size_t expected_total_message_body_len = total_body_bytes;
    size_t actual_total_message_body_len = frameLength - IOHOME_FRAME_CRC_LEN;

    if (expected_total_message_body_len != actual_total_message_body_len) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.printf("[IoHomeNode::parseFrame] ERROR: Message body length mismatch. Expected %u, but got %u (frameLength %u - CRC %u)\n",
                      expected_total_message_body_len, actual_total_message_body_len, frameLength, IOHOME_FRAME_CRC_LEN);
#endif
        return false;
    }

    // 6. Extract Payload
    size_t overhead = IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + actual_security_footer_len;
    if (actual_total_message_body_len < overhead) {
        return false; // Malformed frame length
    }
    size_t actual_payload_len = actual_total_message_body_len - overhead;

    if (actual_payload_len > 0) {
        parsedFrame.payload.resize(actual_payload_len);
        std::copy(frame + offset, frame + offset + actual_payload_len, parsedFrame.payload.begin());
        offset += actual_payload_len;
    }

    // 7. Extract & Verify Security Footer (Counter + MAC)
    // Offset is now correctly pointing to the start of the security footer
    uint16_t rxCounter = (frame[offset] << 8) | frame[offset + 1];
    offset += 2;

    uint8_t rxMac[IOHOME_SECURITY_MAC_LEN] = {0}; // Use max possible MAC length for buffer
    // Copy only the actual MAC bytes based on actual_security_mac_len
    if (actual_security_mac_len > 0) {
        std::copy(frame + offset, frame + offset + actual_security_mac_len, rxMac);
    }
    // Note: No need to increment offset further, we are at the CRC which was already checked.

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x%04X, MAC (first %u bytes): ", rxCounter, (unsigned int)actual_security_mac_len);
    for(size_t i=0; i<actual_security_mac_len; i++) {
        Serial.printf("%02X ", rxMac[i]);
    }
    Serial.println();
#endif

    // --- CRYPTOGRAPHIC VERIFICATION ---
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    uint8_t input_block[16] = {0};
    // The input block for AES is always 16 bytes.
    // It's derived from the first 8 bytes of the frame (Ctrl0, Ctrl1, SrcMAC, DestMAC),
    // followed by the Command ID, the 2-byte rolling counter, and then padding to 16 bytes.
    for(int i = 0; i < 8; i++) { input_block[i] = frame[i]; }
    input_block[8] = parsedFrame.commandId;
    input_block[9] = (uint8_t)(rxCounter >> 8);
    input_block[10] = (uint8_t)(rxCounter & 0xFF);

    // Calculate the expected MAC based on the full 6-byte MAC length for comparison.
    // If the actual frame has a shorter MAC, we will compare only the available bytes.
    uint8_t expected_block[16] = {0};
    mbedtls_aes_setkey_enc(&aes, this->_stack_key, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input_block, expected_block);

    bool macMatch = true;
    // Compare only the number of MAC bytes we actually received
    for (size_t i = 0; i < actual_security_mac_len; i++) {
        if (rxMac[i] != expected_block[i]) {
            macMatch = false;
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
            Serial.printf("[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte %u: Received 0x%02X, Expected 0x%02X\n", i, rxMac[i], expected_block[i]);
#endif
            break;
        }
    }
    mbedtls_aes_free(&aes);

    if (!macMatch) {
        parsedFrame.isValid = false;
        return false;
    }

    // Success
    parsedFrame.isValid = true;
    return true;
}

int16_t IoHomeNode::transmitFrame(const std::vector<uint8_t>& frame) {

    // float freq = this->_channel->c0 + (this->_channel->c1 / 100.0);
    // #if defined(ARDUINO) && defined(DEBUG_IOHOME)
    // Serial.printf("[IoHomeNode::transmitFrame] Setting frequency to %.2f MHz (Channel C0:%u, C1:%u)\n", freq, this->_channel->c0, this->_channel->c1);
    // #endif
    // // Set frequency according to the current channel
    // int16_t state = this->_phyLayer->setFrequency(freq);
    // RADIOLIB_ASSERT(state);

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::transmitFrame] Attempting to transmit frame (len %u): ", frame.size());
    for (size_t i = 0; i < frame.size(); ++i) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println(""); // Use empty string for new line
#endif
    // Transmit the frame
    int16_t state = this->_phyLayer->startTransmit(const_cast<uint8_t*>(frame.data()), frame.size());
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
    // 1. Check for incoming data
    size_t packetLength = this->_phyLayer->getPacketLength();

    if (packetLength == 0) {
        return RADIOLIB_ERR_RX_TIMEOUT;
    }

    // Ghost Packet Detection:
    // The PhysicalLayer abstraction doesn't expose getRSSI(false), so we must
    // downcast to the specific SX126x implementation to access it.
    // NOTE: We use static_cast because RTTI (and thus dynamic_cast) is disabled
    // in the Arduino environment for performance reasons. This is safe as long
    // as we guarantee that the PhysicalLayer object passed to the IoHomeNode
    // constructor is an instance of SX126x or one of its derived classes.
    SX126x* sx126x_radio = static_cast<SX126x*>(this->_phyLayer);
    if (sx126x_radio) {
        float instantaneousRssi = sx126x_radio->getRSSI(false); // 'false' for instantaneous
        if (instantaneousRssi < -100.0) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
            Serial.printf("[IoHomeNode::receiveFrame] Discarding ghost packet (RSSI: %.2f dBm, Len: %u)\n", instantaneousRssi, packetLength);
#endif
            // Forcefully reset the radio's state to clear the FIFO and any stuck IRQ flags.
            // This is more robust than just calling startReceive().
            this->_phyLayer->standby();
            this->_phyLayer->startReceive();
            return RADIOLIB_ERR_RX_TIMEOUT;
        }
    }

    // 2. Read the data
    std::vector<uint8_t> rxBuffer(packetLength);
    int16_t readState = this->_phyLayer->readData(rxBuffer.data(), packetLength);
    if (readState != RADIOLIB_ERR_NONE) {
        // If read fails, something is very wrong. Reset and get out.
        this->_phyLayer->standby();
        this->_phyLayer->startReceive();
        return readState;
    }

    // De-whiten the received data in software.
    // This is necessary because the transmitter's whitening algorithm is likely
    // incompatible with the SX126x's hardware implementation.
    IoHomeNode::deWhiten(rxBuffer.data(), rxBuffer.size());

    // 5. Now, parse the buffer we just read.
    if (!this->parseFrame(rxBuffer.data(), rxBuffer.size(), receivedFrame)) {
        // PARSE FAILED (bad CRC, etc.)
        // This is a critical failure point. The radio might be in a weird state.
        // Forcefully reset it to prevent subsequent noise from being detected as packets.
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("[IoHomeNode::receiveFrame] Packet failed validation (CRC/Parse). Force-resetting radio state.");
#endif
        this->_phyLayer->standby();
        this->_phyLayer->startReceive();
        return RADIOLIB_ERR_CRC_MISMATCH;
    }

    // 6. SUCCESS! The packet was valid.
    // Now we can safely restart the receiver for the next packet.
    this->_phyLayer->startReceive();
    return RADIOLIB_ERR_NONE; // Success!
}

int16_t IoHomeNode::sendWink(NodeId targetMac) {
    // 0x10 = Standard 1-way / 2-way control bits (adjust as needed)
    // 0x01 = Specific control flags
    uint8_t ctrl0 = 0x10;
    uint8_t ctrl1 = 0x01;
    uint8_t commandId = 0x20; // The "Wink" Command

    // Empty payload for a standard Wink
    std::vector<uint8_t> emptyPayload;

    // Generate the cryptographically signed frame
    std::vector<uint8_t> frame = this->buildFrame(
        ctrl0,
        ctrl1,
        this->_source_node_id,
        targetMac,
        commandId,
        emptyPayload
    );

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode] Sending WINK to %02X%02X%02X\n",
                  targetMac.n0, targetMac.n1, targetMac.n2);
#endif

    // Send it over the radio
    return this->transmitFrame(frame);
}

bool IoHomeNode::loop(IoHomeFrame_t& rxFrame) {
    // 1. Check if the radio has received a packet
    // This calls your existing receiveFrame logic
    int16_t state = this->receiveFrame(rxFrame);

    if (state == RADIOLIB_ERR_NONE && rxFrame.isValid) {
        // We found a valid, cryptographically signed frame!
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.printf("[Listener] Captured Cmd: 0x%02X from %02X%02X%02X\n",
                      rxFrame.commandId,
                      rxFrame.sourceMac.n0, rxFrame.sourceMac.n1, rxFrame.sourceMac.n2);
#endif
        return true;
    }

    return false;
}
