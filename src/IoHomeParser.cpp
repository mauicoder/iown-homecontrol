#include "IoHomeParser.h"
#include "IoHome.h"
#include <algorithm>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

uint16_t IoHomeParser::crc16(const uint8_t* data, size_t length) {
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

bool IoHomeParser::validateFrameCrc(const uint8_t* frame, size_t frameLength) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
  Serial.printf("[IoHomeParser::validateFrameCrc] Validating CRC for frame of length: %u\n", frameLength);
#endif
  if (frameLength < IOHOME_FRAME_CRC_LEN) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeParser::validateFrameCrc] Frame too short (len %u) to contain CRC.\n", frameLength);
#endif
    return false;
  }

  uint16_t calculatedCrc = crc16(frame, IOHOME_FRAME_CRC_POS(frameLength));
  uint16_t receivedCrc = frame[IOHOME_FRAME_CRC_POS(frameLength)] | (frame[IOHOME_FRAME_CRC_POS(frameLength) + 1] << 8);

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
  Serial.printf("[IoHomeParser::validateFrameCrc] Calculated CRC: 0x%04X, Received CRC: 0x%04X\n", calculatedCrc, receivedCrc);
#endif
  return calculatedCrc == receivedCrc;
}

bool IoHomeParser::decodeHeader(const uint8_t* frame, size_t frameLength, IoHomeFrame_t& parsedFrame, size_t& outActualMacLen, uint16_t& outRxCounter, uint8_t* outRxMac) {
    parsedFrame.isValid = false;
    parsedFrame.payload.clear();

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeParser::decodeHeader] --- Starting parse for frame of length: %u ---\n", (unsigned int)frameLength);
    Serial.print("[IoHomeParser::decodeHeader] Raw bytes: ");
    for(size_t i=0; i<frameLength; i++) { Serial.printf("%02X ", frame[i]); }
    Serial.println();
#endif

    if (frameLength < (IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + IOHOME_SECURITY_COUNTER_LEN + IOHOME_FRAME_CRC_LEN)) return false;
    if (!validateFrameCrc(frame, frameLength)) return false;

    size_t offset = 0;
    parsedFrame.ctrlByte0 = frame[offset++];
    parsedFrame.ctrlByte1 = frame[offset++];
    parsedFrame.destMac.n0 = frame[offset++];
    parsedFrame.destMac.n1 = frame[offset++];
    parsedFrame.destMac.n2 = frame[offset++];
    parsedFrame.sourceMac.n0 = frame[offset++];
    parsedFrame.sourceMac.n1 = frame[offset++];
    parsedFrame.sourceMac.n2 = frame[offset++];
    parsedFrame.commandId = frame[offset++];

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeParser::decodeHeader] Decoded Header: Ctrl0=0x%02X (Binary: %s), Ctrl1=0x%02X (Binary: %s)\n",
                  parsedFrame.ctrlByte0, String(parsedFrame.ctrlByte0, BIN).c_str(),
                  parsedFrame.ctrlByte1, String(parsedFrame.ctrlByte1, BIN).c_str());
    Serial.printf("[IoHomeParser::decodeHeader] Source MAC: %02X%02X%02X, Dest MAC: %02X%02X%02X\n",
                  parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2,
                  parsedFrame.destMac.n0, parsedFrame.destMac.n1, parsedFrame.destMac.n2);
    Serial.printf("[IoHomeParser::decodeHeader] Command ID: 0x%02X\n", parsedFrame.commandId);
#endif

    size_t total_body_bytes = (parsedFrame.ctrlByte0 & 0x1F) + 1;
    outActualMacLen = IOHOME_SECURITY_MAC_LEN;
    if (parsedFrame.commandId == 0x30) {
        outActualMacLen = 2; // 0x30 Command specific short MAC
    }
    size_t actual_security_footer_len = IOHOME_SECURITY_COUNTER_LEN + outActualMacLen;

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.printf("[IoHomeParser::decodeHeader] Assuming security footer length: %u (Counter: %u, MAC: %u)\n",
                  (unsigned int)actual_security_footer_len, (unsigned int)IOHOME_SECURITY_COUNTER_LEN, (unsigned int)outActualMacLen);
#endif

    size_t expected_total_message_body_len = total_body_bytes;
    size_t actual_total_message_body_len = frameLength - IOHOME_FRAME_CRC_LEN;
    if (expected_total_message_body_len != actual_total_message_body_len) return false;

    size_t overhead = IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + actual_security_footer_len;
    if (actual_total_message_body_len < overhead) return false;

    size_t actual_payload_len = actual_total_message_body_len - overhead;
    if (actual_payload_len > 0) {
        parsedFrame.payload.resize(actual_payload_len);
        std::copy(frame + offset, frame + offset + actual_payload_len, parsedFrame.payload.begin());
        offset += actual_payload_len;
    }

    outRxCounter = (frame[offset] << 8) | frame[offset + 1];
    offset += 2;

    if (outActualMacLen > 0) {
        std::copy(frame + offset, frame + offset + outActualMacLen, outRxMac);
    }

    parsedFrame.isValid = true;
    return true;
}
