#include "IoHome.h"
#include "IoHomeParser.h"
#include "IoHomeCrypto.h"
#include <RadioLib.h>
#include <mbedtls/aes.h>
#include <stdexcept>
#include "TypeDef.h"
#include "protocols/PhysicalLayer/PhysicalLayer.h"
#include "hal/BoardHAL.h"
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
  return IoHomeParser::crc16(data, length);
}

bool IoHomeNode::validateFrameCrc(const uint8_t* frame, size_t frameLength) {
  return IoHomeParser::validateFrameCrc(frame, frameLength);
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
  frame[offset++] = destMac.n0;   frame[offset++] = destMac.n1;   frame[offset++] = destMac.n2;
  frame[offset++] = sourceMac.n0; frame[offset++] = sourceMac.n1; frame[offset++] = sourceMac.n2;

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
  uint8_t output_block[16] = {0};

  IoHomeFrame_t tempFrame;
  tempFrame.ctrlByte0 = finalCtrlByte0;
  tempFrame.commandId = commandId;
  tempFrame.payload = payload;

  IoHomeCrypto::generateMac(tempFrame, frame.data(), this->_sequence_counter, this->_stack_key, output_block);

  // 4. Extract 6 bytes and insert into frame
  for (int i = 0; i < IOHOME_SECURITY_MAC_LEN; i++) {
      frame[offset++] = output_block[i];
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
    size_t actual_security_mac_len = 0;
    uint16_t rxCounter = 0;
    uint8_t rxMac[16] = {0};

    if (!IoHomeParser::decodeHeader(frame, frameLength, parsedFrame, actual_security_mac_len, rxCounter, rxMac)) {
        return false;
    }

    // --- KEY TRANSFER (0x30) INTERCEPTION ---
    uint8_t active_mac_key[16];
    std::copy(this->_stack_key, this->_stack_key + 16, active_mac_key);

    if (parsedFrame.commandId == 0x30 && parsedFrame.payload.size() >= 16) {
        uint8_t extracted_key[16];
        IoHomeCrypto::decryptTransferKey(parsedFrame, extracted_key);

        // The 0x30 command's own MAC is calculated using the key that is actively being transferred
        std::copy(extracted_key, extracted_key + 16, active_mac_key);

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("\n=======================================================");
        Serial.println("[IoHomeNode::parseFrame] !!! 1-WAY KEY TRANSFER (0x30) DETECTED !!!");
        Serial.print("[IoHomeNode::parseFrame] Decrypted Stack Key : ");
        for (int i = 0; i < 16; i++) Serial.printf("%02X ", extracted_key[i]);
        Serial.println("\n=======================================================\n");
#endif
    }

    // --- CRYPTOGRAPHIC VERIFICATION ---
    bool macMatch = IoHomeCrypto::verifyMac(parsedFrame, frame, rxCounter, rxMac, actual_security_mac_len, active_mac_key);

    if (!macMatch) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
        Serial.println("[IoHomeNode::parseFrame] --- AES Verification Failed Details ---");
        Serial.print("[IoHomeNode::parseFrame] Stack Key    : ");
        for(int i = 0; i < 16; i++) Serial.printf("%02X ", active_mac_key[i]);
        Serial.println("\n[IoHomeNode::parseFrame] -----------------------------------------");
#endif
        parsedFrame.isValid = false;
        return false;
    }

    // --- Keep ESP32 Sequence Counter in Sync! ---
    // To successfully spoof the remote, our sequence counter must be strictly greater
    // than the last counter the awning received.
    if (rxCounter >= this->_sequence_counter || this->_sequence_counter == 0) {
        this->_sequence_counter = rxCounter + 1;
    }

    // Success
    parsedFrame.isValid = true;
    return true;
}

int16_t IoHomeNode::sendButton(uint16_t buttonAction) {
    int16_t state = RADIOLIB_ERR_NONE;
    bool useBroadcast = (_destination_node_id.n0 == 0 && _destination_node_id.n1 == 0 && _destination_node_id.n2 == 0);

    // 1. WAKE-UP / BROADCAST FRAME
    // Multi-channel remotes (like Situo 5) always start a sequence with a generic 6-byte broadcast.
    // This wakes up sleeping awnings and informs smart hubs (like TaHoma) of the action.
    NodeId broadcastMac = {0x00, 0x00, 0x3F};
    std::vector<uint8_t> broadcastPayload = { 0x01, 0x43, (uint8_t)(buttonAction >> 8), (uint8_t)(buttonAction & 0xFF), 0x00, 0x00 };
    std::vector<uint8_t> broadcastFrame = this->buildFrame(0xF0, 0x00, this->_source_node_id, broadcastMac, IOHOME_CMD_0x00, broadcastPayload);
    state = this->transmitFrame(broadcastFrame);

    // 2. TARGETED EXECUTION FRAME
    // After waking up the network, the remote sends the actual targeted execution command
    // directly to the paired awning using the 8-byte Absolute Parameter (0x80 0xC8).
    if (!useBroadcast) {
        std::vector<uint8_t> targetedPayload;
        if (buttonAction == IOHOME_ACTION_MY) {
            targetedPayload = {0x01, 0x43, (uint8_t)(buttonAction >> 8), (uint8_t)(buttonAction & 0xFF), 0x00, 0x00};
        } else {
            targetedPayload = {0x01, 0x43, (uint8_t)(buttonAction >> 8), (uint8_t)(buttonAction & 0xFF), 0x80, 0xC8, 0x00, 0x00};
        }
        std::vector<uint8_t> targetedFrame = this->buildFrame(0xF0, 0x00, this->_source_node_id, this->_destination_node_id, IOHOME_CMD_0x00, targetedPayload);
        state = this->transmitFrame(targetedFrame);
    }

    return state;
}

int16_t IoHomeNode::transmitFrame(const std::vector<uint8_t>& frame) {

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeNode::transmitFrame] Attempting to transmit frame (len %u): ", frame.size());
    for (size_t i = 0; i < frame.size(); ++i) {
        Serial.printf("%02X ", frame[i]);
    }
    Serial.println(""); // Use empty string for new line
#endif

    std::vector<uint8_t> txBuffer = frame;

    // io-homecontrol is UART-encoded (8N1) over the air.
    // Since we receive the raw FSK bitstream and software-decode the UART frames in receiveFrame,
    // we must also manually UART-encode our transmission buffer so the awning can understand it.
    std::vector<uint8_t> uartBuffer;

    // --- HARDWARE UART ALIGNMENT ---
    // Before sending the first Start Bit, the RF line MUST be perfectly IDLE (High).
    // The SX1262 natively ends the Sync Word and instantly begins the payload.
    // Prepending 2 bytes of 0xFF ensures the awning's physical hardware UART decoder
    // is fully synchronized and waiting for the first Start Bit.
    uartBuffer.push_back(0xFF);
    uartBuffer.push_back(0xFF);

    uint16_t currentTxByte = 0;
    int txBits = 0;

    auto pushBit = [&](uint8_t bit) {
        currentTxByte = (currentTxByte << 1) | (bit & 0x01);
        txBits++;
        if (txBits == 8) {
            uartBuffer.push_back((uint8_t)currentTxByte);
            currentTxByte = 0;
            txBits = 0;
        }
    };

    for (size_t i = 0; i < txBuffer.size(); i++) {
        uint8_t b = txBuffer[i];
        pushBit(0); // Start bit
        for (int j = 0; j < 8; j++) {
            pushBit((b >> j) & 0x01); // LSB first
        }
        pushBit(1); // Stop bit
    }
    while (txBits > 0) { pushBit(1); } // Pad final byte with Idle (1)

    // Append 2 bytes of IDLE (0xFF) to the end to guarantee the radio
    // doesn't cut off the transmission power before the final Stop Bit completes.
    uartBuffer.push_back(0xFF);
    uartBuffer.push_back(0xFF);

    // Transmit the frame 12 times across all 3 frequencies to mimic the physical remote
    // and hit the awning's wake window regardless of which channel it is currently scanning.
    int16_t state = RADIOLIB_ERR_NONE;
    const float freqs[3] = { 868.25f, 868.95f, 869.85f };

    for (int i = 0; i < 12; i++) {
        if (this->_phyLayer) {
            this->_phyLayer->standby();
            this->_phyLayer->setFrequency(freqs[i % 3]);
        }
        state = this->_phyLayer->transmit(uartBuffer.data(), uartBuffer.size());
        if (i < 11) delay(30); // 30ms gap saturates the awning's wake window
    }

    // Restore original frequency
    if (this->_phyLayer && this->_channel) {
        this->_phyLayer->standby();
        this->_phyLayer->setFrequency(this->_channel->c0 + (this->_channel->c1 / 100.0f));
    }
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[IoHomeNode::transmitFrame] Transmission sequence completed successfully.");
    } else {
        Serial.printf("[IoHomeNode::transmitFrame] Transmission failed with error: %d\n", state);
    }
#endif
    return state;
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
