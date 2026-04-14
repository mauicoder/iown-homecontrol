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
    _channel(channel_param) {
    memset(_profiles, 0, sizeof(_profiles));
}

void IoHomeNode::loadProfiles(IoHomeProfile* profiles, size_t count) {
    _numProfiles = std::min(count, (size_t)5);
    for(size_t i = 0; i < _numProfiles; i++) {
        _profiles[i] = profiles[i];
    }
}

uint16_t IoHomeNode::crc16(const uint8_t* data, size_t length) {
  return IoHomeParser::crc16(data, length);
}

bool IoHomeNode::validateFrameCrc(const uint8_t* frame, size_t frameLength) {
  return IoHomeParser::validateFrameCrc(frame, frameLength);
}

std::vector<uint8_t> IoHomeNode::buildFrame(
  uint8_t ctrlByte0, uint8_t ctrlByte1,
  uint8_t commandId, const std::vector<uint8_t>& payload,
  IoHomeProfile& profile
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
  frame[offset++] = profile.destMac.n0;   frame[offset++] = profile.destMac.n1;   frame[offset++] = profile.destMac.n2;
  frame[offset++] = profile.sourceMac.n0; frame[offset++] = profile.sourceMac.n1; frame[offset++] = profile.sourceMac.n2;

  // Command
  frame[offset++] = commandId;

  // Payload
  if (!payload.empty()) {
    std::copy(payload.begin(), payload.end(), frame.begin() + offset);
    offset += payload.size();
  }

  // --- SECURITY FOOTER SECTION (Layer 3) ---

  // A: Insert the 2-byte Rolling Counter (Big Endian: High Byte, then Low Byte)
  frame[offset++] = (uint8_t)((profile.seqCounter >> 8) & 0xFF); // High Byte
  frame[offset++] = (uint8_t)(profile.seqCounter & 0xFF);        // Low Byte

  // --- LAYER 3: AES-128 MAC (Message Authentication Code) ---
  uint8_t output_block[16] = {0};

  IoHomeFrame_t tempFrame;
  tempFrame.ctrlByte0 = finalCtrlByte0;
  tempFrame.commandId = commandId;
  tempFrame.payload = payload;

  IoHomeCrypto::generateMac(tempFrame, frame.data(), profile.seqCounter, profile.stackKey, output_block);

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
  profile.seqCounter++;

  return frame;
}

bool IoHomeNode::parseFrame(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame) {
    size_t actual_security_mac_len = 0;
    uint16_t rxCounter = 0;
    uint8_t rxMac[16] = {0};

    if (!IoHomeParser::decodeHeader(frame, frameLength, parsedFrame, actual_security_mac_len, rxCounter, rxMac)) {
        return false;
    }

    // --- FIND MATCHING PROFILE ---
    IoHomeProfile* activeProfile = nullptr;
    for (size_t i = 0; i < _numProfiles; i++) {
        if (_profiles[i].active &&
            _profiles[i].sourceMac.n0 == parsedFrame.sourceMac.n0 &&
            _profiles[i].sourceMac.n1 == parsedFrame.sourceMac.n1 &&
            _profiles[i].sourceMac.n2 == parsedFrame.sourceMac.n2) {
            activeProfile = &_profiles[i];
            break;
        }
    }

    // --- KEY TRANSFER (0x30) INTERCEPTION ---
    uint8_t active_mac_key[16] = {0};
    if (activeProfile) {
        std::copy(activeProfile->stackKey, activeProfile->stackKey + 16, active_mac_key);
    }

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
    if (activeProfile) {
        if (rxCounter >= activeProfile->seqCounter || activeProfile->seqCounter == 0) {
            activeProfile->seqCounter = rxCounter + 1;
        }
    }

    // Success
    parsedFrame.isValid = true;
    return true;
}

int16_t IoHomeNode::sendButton(uint16_t buttonAction, uint8_t profileIndex) {
    if (profileIndex >= _numProfiles) return RADIOLIB_ERR_INVALID_NUM_SAMPLES;
    IoHomeProfile& profile = _profiles[profileIndex];

    int16_t state = RADIOLIB_ERR_NONE;
    bool useBroadcast = (profile.destMac.n0 == 0 && profile.destMac.n1 == 0 && profile.destMac.n2 == 0);

    NodeId originalDest = profile.destMac;

    // 1. WAKE-UP / BROADCAST FRAME
    // Multi-channel remotes (like Situo 5) always start a sequence with a generic 6-byte broadcast.
    // This wakes up sleeping awnings and informs smart hubs (like TaHoma) of the action.
    profile.destMac = {0x00, 0x00, 0x3F};
    std::vector<uint8_t> broadcastPayload = { 0x01, 0x43, (uint8_t)(buttonAction >> 8), (uint8_t)(buttonAction & 0xFF), 0x00, 0x00 };
    std::vector<uint8_t> broadcastFrame = this->buildFrame(0xF0, 0x00, IOHOME_CMD_0x00, broadcastPayload, profile);
    state = this->transmitFrame(broadcastFrame);

    // Restore original destination MAC
    profile.destMac = originalDest;

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
        // 0x60 = 2-Way Mode (Bit 7 = 0). Forces the awning to generate a reply packet!
        std::vector<uint8_t> targetedFrame = this->buildFrame(0x60, 0x00, IOHOME_CMD_0x00, targetedPayload, profile);
        state = this->transmitFrame(targetedFrame);
    }

    return state;
}

int16_t IoHomeNode::pollStatus(uint8_t profileIndex) {
    if (profileIndex >= _numProfiles) return RADIOLIB_ERR_INVALID_NUM_SAMPLES;
    IoHomeProfile& profile = _profiles[profileIndex];

    // 1. WAKE-UP / BROADCAST FRAME
    // Awning receivers sleep to save power. We broadcast the poll first to wake them up.
    NodeId originalDest = profile.destMac;
    profile.destMac = {0x00, 0x00, 0x3F};
    std::vector<uint8_t> targetedPayload = {};
    std::vector<uint8_t> broadcastFrame = this->buildFrame(0xF0, 0x00, 0x54, targetedPayload, profile);
    this->transmitFrame(broadcastFrame);

    // Restore original destination MAC
    profile.destMac = originalDest;

    // 2. TARGETED POLL FRAME
    // Send Command 0x54 (Get General Info 1) in 2-Way Mode (0x60)
    std::vector<uint8_t> targetedFrame = this->buildFrame(0x60, 0x00, 0x54, targetedPayload, profile);
    return this->transmitFrame(targetedFrame);
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

    // Transmit the frame 18 times across all 3 frequencies to mimic the physical remote
    // and hit the awning's wake window regardless of which channel it is currently scanning.
    int16_t state = RADIOLIB_ERR_NONE;
    const float freqs[3] = { 868.25f, 868.95f, 869.85f };

    for (int i = 0; i < 18; i++) {
        if (this->_phyLayer) {
            this->_phyLayer->standby();
            this->_phyLayer->setFrequency(freqs[i % 3]);
        }
        state = this->_phyLayer->transmit(uartBuffer.data(), uartBuffer.size());
        if (i < 17) delay(30); // 30ms gap saturates the awning's wake window
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
