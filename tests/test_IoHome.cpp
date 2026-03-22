// Define missing RadioLib constants for testing environment if not pulled in
#ifndef RADIOLIB_IRQ_ALL
#define RADIOLIB_IRQ_ALL 0xFFFFFFFFU // A common full mask
#endif



//tests for IoHomeNode CRC16 calculation and frame building/parsing

#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip> // For std::hex, std::setw, std::setfill
#include <cmath>   // Required for std::abs

// Include the header for the function we want to test
#include "IoHome.h"
#include <RadioLib.h> // Explicitly include for RadioLib error codes
#include "RadioMock.h"
#include "Common.h"
#include "test_IoHome.h"
#include "TestHelpers.h"

void runCRCTests()
{
    // Test case 1: Empty data
    std::vector<uint8_t> data1 = {};
    uint16_t expected_crc1 = 0x0000;
    uint16_t actual_crc1 = IoHomeNode::crc16(data1.data(), data1.size());
    runTest("Empty data", actual_crc1 == expected_crc1, "", actual_crc1, expected_crc1);

    // Test case 2: Single byte 0x00
    std::vector<uint8_t> data2 = {0x00};
    uint16_t expected_crc2 = 0x0000;
    uint16_t actual_crc2 = IoHomeNode::crc16(data2.data(), data2.size());
    runTest("Single byte 0x00", actual_crc2 == expected_crc2, "", actual_crc2, expected_crc2);

    // Test case 3: Single byte 0x01
    std::vector<uint8_t> data3 = {0x01};
    uint16_t expected_crc3 = 0x1189; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc3 = IoHomeNode::crc16(data3.data(), data3.size());
    runTest("Single byte 0x01", actual_crc3 == expected_crc3, "", actual_crc3, expected_crc3);

    // Test case 4: Single byte 0x10
    std::vector<uint8_t> data4 = {0x10};
    uint16_t expected_crc4 = 0x1081; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc4 = IoHomeNode::crc16(data4.data(), data4.size());
    runTest("Single byte 0x10", actual_crc4 == expected_crc4, "", actual_crc4, expected_crc4);

    // Test case 5: "123456789" ASCII
    std::vector<uint8_t> data5 = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint16_t expected_crc5 = 0x2189; // Calculated using online CRC-16/KERMIT calculator (poly 0x1021, init 0x0000, no reverse, no xor out)
    uint16_t actual_crc5 = IoHomeNode::crc16(data5.data(), data5.size());
    runTest("ASCII '123456789'", actual_crc5 == expected_crc5, "", actual_crc5, expected_crc5);

    // Test case 6: "DEADBEEF" hex
    std::vector<uint8_t> data6 = {0xDE, 0xAD, 0xBE, 0xEF};
    uint16_t expected_crc6 = 0x1915; // Corrected expected value for CRC-16/KERMIT (poly 0x1021, init 0x0000, no reflection, no xor out)
    uint16_t actual_crc6 = IoHomeNode::crc16(data6.data(), data6.size());
    runTest("Hex DEAD BEEF", actual_crc6 == expected_crc6, "CRC mismatch for DEADBEEF", actual_crc6, expected_crc6);

    // Test case 7: Data length 10, all zeros
    std::vector<uint8_t> data7(10, 0x00);
    uint16_t expected_crc7 = 0x0000;
    uint16_t actual_crc7 = IoHomeNode::crc16(data7.data(), data7.size());
    runTest("10 zero bytes", actual_crc7 == expected_crc7, "", actual_crc7, expected_crc7);

    // Test case 8: Data length 10, all ones (0xFF)
    std::vector<uint8_t> data8(10, 0xFF);
    uint16_t expected_crc8 = 0x1BE2; // Corrected expected value for CRC-16/KERMIT, 10 bytes of 0xFF
    uint16_t actual_crc8 = IoHomeNode::crc16(data8.data(), data8.size());
    runTest("10 0xFF bytes", actual_crc8 == expected_crc8, "", actual_crc8, expected_crc8);

    // Test case 9: Alternating bytes (0x55, 0xAA, 0x55, 0xAA)
    std::vector<uint8_t> data9 = {0x55, 0xAA, 0x55, 0xAA};
    uint16_t expected_crc9 = 0x60F3; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc9 = IoHomeNode::crc16(data9.data(), data9.size());
    runTest("Alternating 0x55/0xAA bytes", actual_crc9 == expected_crc9, "", actual_crc9, expected_crc9);

    // Test case 10: Longer string "The quick brown fox jumps over the lazy dog."
    std::string str10 = "The quick brown fox jumps over the lazy dog.";
    std::vector<uint8_t> data10(str10.begin(), str10.end());
    uint16_t expected_crc10 = 0x07FC; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc10 = IoHomeNode::crc16(data10.data(), data10.size());
    runTest("Longer string 'The quick brown fox...'", actual_crc10 == expected_crc10, "", actual_crc10, expected_crc10);
}

void runFrameTests()
{
    IoHomeTestFixture fx; // All values initialized once here

    // --- Test IOHOME_MSG_LEN macro ---
    // A message with length 0x05 (0b00101) in CTRLBYTE0 would mean 5 + 1 = 6 bytes message length
    // Frame: [CTRLBYTE0 (0x05), CTRLBYTE1, MAC_SRC, MAC_DEST, COMMAND, PARAMETER]
    std::vector<uint8_t> dummy_frame_len_5 = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t expected_msg_len_5 = (0x05 & 0x1F) + 1; // Expected: 6
    runTest("IOHOME_MSG_LEN (length 5)", IOHOME_MSG_LEN(dummy_frame_len_5.data()) == expected_msg_len_5,
            "Macro length mismatch", IOHOME_MSG_LEN(dummy_frame_len_5.data()), expected_msg_len_5);

    // A message with length 0x1F (0b11111) in CTRLBYTE0 would mean 31 + 1 = 32 bytes message length
    std::vector<uint8_t> dummy_frame_len_1F = {0x1F}; // Max length code
    size_t expected_msg_len_1F = (0x1F & 0x1F) + 1; // Expected: 32
    runTest("IOHOME_MSG_LEN (length 1F)", IOHOME_MSG_LEN(dummy_frame_len_1F.data()) == expected_msg_len_1F,
            "Macro length mismatch", IOHOME_MSG_LEN(dummy_frame_len_1F.data()), expected_msg_len_1F);

    // --- Test validateFrameCrc ---

    // Test case 11: Valid frame with calculated CRC
    // Data: {0x01, 0x02, 0x03, 0x04}
    // Expected CRC-16/KERMIT for {0x01, 0x02, 0x03, 0x04} is 0x76BC
    std::vector<uint8_t> valid_frame_data = {0x01, 0x02, 0x03, 0x04};
    uint16_t calculated_valid_crc = IoHomeNode::crc16(valid_frame_data.data(), valid_frame_data.size());

    std::vector<uint8_t> frame_with_valid_crc = valid_frame_data;
    frame_with_valid_crc.push_back((uint8_t)(calculated_valid_crc & 0xFF));        // LSB
    frame_with_valid_crc.push_back((uint8_t)((calculated_valid_crc >> 8) & 0xFF)); // MSB

    runTest("validateFrameCrc (valid frame)", IoHomeNode::validateFrameCrc(frame_with_valid_crc.data(), frame_with_valid_crc.size()));

    // Test case 12: Invalid frame (data corrupted)
    std::vector<uint8_t> frame_with_corrupted_data = valid_frame_data;
    frame_with_corrupted_data[0] = 0xFF; // Corrupt data
    frame_with_corrupted_data.push_back((uint8_t)(calculated_valid_crc & 0xFF));
    frame_with_corrupted_data.push_back((uint8_t)((calculated_valid_crc >> 8) & 0xFF));
    runTest("validateFrameCrc (data corrupted)", !IoHomeNode::validateFrameCrc(frame_with_corrupted_data.data(), frame_with_corrupted_data.size()));

    // Test case 13: Invalid frame (CRC corrupted)
    std::vector<uint8_t> frame_with_corrupted_crc = valid_frame_data;
    frame_with_corrupted_crc.push_back(0x00); // Corrupt LSB of CRC
    frame_with_corrupted_crc.push_back(0x00); // Corrupt MSB of CRC
    runTest("validateFrameCrc (CRC corrupted)", !IoHomeNode::validateFrameCrc(frame_with_corrupted_crc.data(), frame_with_corrupted_crc.size()));

    // Test case 14: Frame too short for CRC
    std::vector<uint8_t> short_frame = {0x01}; // Length 1, CRC is 2 bytes
    runTest("validateFrameCrc (frame too short)", !IoHomeNode::validateFrameCrc(short_frame.data(), short_frame.size()));

    std::cout << "\nStarting IoHomeNode Frame Building tests..." << std::endl;

    // Test case 15: Build frame with no payload
    std::vector<uint8_t> no_payload_vec = {};
    std::vector<uint8_t> frame_no_payload = IoHomeNode::buildFrame(
      fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, no_payload_vec
    );
    size_t expected_message_body_len_no_payload = IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN; // 8 + 1 = 9
    size_t expected_total_frame_len_no_payload = expected_message_body_len_no_payload + IOHOME_FRAME_CRC_LEN; // 9 + 2 = 11

    runTest("buildFrame (no payload) - total length", frame_no_payload.size() == expected_total_frame_len_no_payload,
            "Total length mismatch", frame_no_payload.size(), expected_total_frame_len_no_payload);
    runTest("buildFrame (no payload) - CRC valid", IoHomeNode::validateFrameCrc(frame_no_payload.data(), frame_no_payload.size()));
    // Verify specific bytes
    runTest("buildFrame (no payload) - CTRL0 length bits", (frame_no_payload[IOHOME_CTRLBYTE0_POS] & 0x1F) == (expected_message_body_len_no_payload - 1),
            "CTRL0 length bits mismatch", (frame_no_payload[IOHOME_CTRLBYTE0_POS] & 0x1F), (expected_message_body_len_no_payload - 1)); // 9 - 1 = 8
    runTest("buildFrame (no payload) - CTRL0 mode/order bits", (frame_no_payload[IOHOME_CTRLBYTE0_POS] & ~0x1F) == fx.ctrl0_base,
            "CTRL0 mode/order bits mismatch", (frame_no_payload[IOHOME_CTRLBYTE0_POS] & ~0x1F), fx.ctrl0_base);
    runTest("buildFrame (no payload) - CTRL1", frame_no_payload[IOHOME_CTRLBYTE1_POS] == fx.ctrl1,
            "CTRL1 mismatch", frame_no_payload[IOHOME_CTRLBYTE1_POS], fx.ctrl1);
    runTest("buildFrame (no payload) - SRC_MAC[0]", frame_no_payload[IOHOME_MAC_SOURCE_POS] == fx.src_mac.n0,
            "SRC_MAC[0] mismatch", frame_no_payload[IOHOME_MAC_SOURCE_POS], fx.src_mac.n0);

    runTest("buildFrame (no payload) - DEST_MAC[0]", frame_no_payload[IOHOME_MAC_DEST_POS] == fx.dest_mac.n0,
            "DEST_MAC[0] mismatch", frame_no_payload[IOHOME_MAC_DEST_POS], fx.dest_mac.n0);
    runTest("buildFrame (no payload) - CMD_ID", frame_no_payload[IOHOME_FRAME_HEADER_LEN] == fx.cmd_id,
            "CMD_ID mismatch", frame_no_payload[IOHOME_FRAME_HEADER_LEN], fx.cmd_id);
    printHexVector("Built frame (no payload)", frame_no_payload);


    // Test case 16: Build frame with a small payload
    std::vector<uint8_t> small_payload_vec = {0xA0, 0xB1, 0xC2}; // 3 bytes
    std::vector<uint8_t> frame_small_payload = IoHomeNode::buildFrame(
            fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, small_payload_vec
    );
    size_t expected_message_body_len_small_payload = IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + small_payload_vec.size(); // 8 + 1 + 3 = 12
    size_t expected_total_frame_len_small_payload = expected_message_body_len_small_payload + IOHOME_FRAME_CRC_LEN; // 12 + 2 = 14

    runTest("buildFrame (small payload) - total length", frame_small_payload.size() == expected_total_frame_len_small_payload,
            "Total length mismatch", frame_small_payload.size(), expected_total_frame_len_small_payload);
    runTest("buildFrame (small payload) - CRC valid", IoHomeNode::validateFrameCrc(frame_small_payload.data(), frame_small_payload.size()));
    // Verify specific bytes
    runTest("buildFrame (small payload) - CTRL0 length bits", (frame_small_payload[IOHOME_CTRLBYTE0_POS] & 0x1F) == (expected_message_body_len_small_payload - 1),
            "CTRL0 length bits mismatch", (frame_small_payload[IOHOME_CTRLBYTE0_POS] & 0x1F), (expected_message_body_len_small_payload - 1)); // 12 - 1 = 11
    runTest("buildFrame (small payload) - Payload[0]", frame_small_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN] == 0xA0,
            "Payload[0] mismatch", frame_small_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN], 0xA0);
    runTest("buildFrame (small payload) - Payload[2]", frame_small_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + 2] == 0xC2,
            "Payload[2] mismatch", frame_small_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + 2], 0xC2);
    printHexVector("Built frame (small payload)", frame_small_payload);


    // Test case 17: Build frame with maximum possible payload length
    // The length field in CTRLBYTE0 is 5 bits, so max value is 31 (0x1F).
    // This means max messageBodyLen = 31 + 1 = 32 bytes.
    // messageBodyLen = IOHOME_FRAME_HEADER_LEN (8) + IOHOME_COMMAND_ID_LEN (1) + payload.size()
    // 32 = 8 + 1 + payload.size() => payload.size() = 23
    size_t max_payload_size = 23;
    std::vector<uint8_t> max_payload_vec(max_payload_size, 0xDE); // Max payload 23 bytes
    std::vector<uint8_t> frame_max_payload = IoHomeNode::buildFrame(
      fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, max_payload_vec
    );
    size_t expected_message_body_len_max_payload = IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + max_payload_size; // 8 + 1 + 23 = 32
    size_t expected_total_frame_len_max_payload = expected_message_body_len_max_payload + IOHOME_FRAME_CRC_LEN; // 32 + 2 = 34

    runTest("buildFrame (max payload) - total length", frame_max_payload.size() == expected_total_frame_len_max_payload,
            "Total length mismatch", frame_max_payload.size(), expected_total_frame_len_max_payload);
    runTest("buildFrame (max payload) - CRC valid", IoHomeNode::validateFrameCrc(frame_max_payload.data(), frame_max_payload.size()));
    runTest("buildFrame (max payload) - CTRL0 length bits", (frame_max_payload[IOHOME_CTRLBYTE0_POS] & 0x1F) == (expected_message_body_len_max_payload - 1),
            "CTRL0 length bits mismatch", (frame_max_payload[IOHOME_CTRLBYTE0_POS] & 0x1F), (expected_message_body_len_max_payload - 1)); // 32 - 1 = 31 = 0x1F
    runTest("buildFrame (max payload) - Payload[0]", frame_max_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN] == 0xDE,
            "Payload[0] mismatch", frame_max_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN], 0xDE);
    runTest("buildFrame (max payload) - Last Payload byte", frame_max_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + max_payload_size - 1] == 0xDE,
            "Last Payload byte mismatch", frame_max_payload[IOHOME_FRAME_HEADER_LEN + IOHOME_COMMAND_ID_LEN + max_payload_size - 1], 0xDE);
    printHexVector("Built frame (max payload)", frame_max_payload);

    std::cout << "All IoHomeNode Frame Building tests completed." << std::endl;

    std::cout << "\nStarting IoHomeNode Frame Parsing tests..." << std::endl;

    IoHomeFrame_t parsed_frame;

    // Test case 18: Parse a valid frame with no payload
    std::vector<uint8_t> parsed_frame_no_payload = IoHomeNode::buildFrame(
      fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, no_payload_vec
    );
    printHexVector("Frame to parse (no payload)", parsed_frame_no_payload);
    bool parse_result_no_payload = IoHomeNode::parseFrame(parsed_frame_no_payload.data(), parsed_frame_no_payload.size(), parsed_frame);
    runTest("parseFrame (no payload) - overall result", parse_result_no_payload);
    runTest("parseFrame (no payload) - isValid flag", parsed_frame.isValid);
    runTest("parseFrame (no payload) - CTRL0", parsed_frame.ctrlByte0 == parsed_frame_no_payload[IOHOME_CTRLBYTE0_POS],
            "CTRL0 mismatch", parsed_frame.ctrlByte0, parsed_frame_no_payload[IOHOME_CTRLBYTE0_POS]);
    runTest("parseFrame (no payload) - CTRL1", parsed_frame.ctrlByte1 == fx.ctrl1,
            "CTRL1 mismatch", parsed_frame.ctrlByte1, fx.ctrl1);
    runTest("parseFrame (no payload) - SRC_MAC[0]", parsed_frame.sourceMac.n0 == fx.src_mac.n0,
            "SRC_MAC[0] mismatch", parsed_frame.sourceMac.n0, fx.src_mac.n0);
    runTest("parseFrame (no payload) - DEST_MAC[0]", parsed_frame.destMac.n0 == fx.dest_mac.n0,
            "DEST_MAC[0] mismatch", parsed_frame.destMac.n0, fx.dest_mac.n0);
    runTest("parseFrame (no payload) - CMD_ID", parsed_frame.commandId == fx.cmd_id,
            "CMD_ID mismatch", parsed_frame.commandId, fx.cmd_id);
    runTest("parseFrame (no payload) - Payload size", parsed_frame.payload.empty(),
            "Payload size mismatch (expected empty)", parsed_frame.payload.size(), (size_t)0);

    // Test case 19: Parse a valid frame with a small payload
    std::vector<uint8_t> parsed_frame_small_payload = IoHomeNode::buildFrame(
      fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, small_payload_vec
    );
    printHexVector("Frame to parse (small payload)", parsed_frame_small_payload);
    bool parse_result_small_payload = IoHomeNode::parseFrame(parsed_frame_small_payload.data(), parsed_frame_small_payload.size(), parsed_frame);
    runTest("parseFrame (small payload) - overall result", parse_result_small_payload);
    runTest("parseFrame (small payload) - isValid flag", parsed_frame.isValid);
    runTest("parseFrame (small payload) - Payload size", parsed_frame.payload.size() == small_payload_vec.size(),
            "Payload size mismatch", parsed_frame.payload.size(), small_payload_vec.size());
    runTest("parseFrame (small payload) - Payload content", parsed_frame.payload == small_payload_vec);
    if (!(parsed_frame.payload == small_payload_vec)) {
        printHexVector("    Expected payload", small_payload_vec);
        printHexVector("    Actual payload", parsed_frame.payload);
    }

    // Test case 20: Parse a valid frame with maximum payload
    std::vector<uint8_t> parsed_frame_max_payload = IoHomeNode::buildFrame(
      fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, max_payload_vec
    );
    printHexVector("Frame to parse (max payload)", parsed_frame_max_payload);
    bool parse_result_max_payload = IoHomeNode::parseFrame(parsed_frame_max_payload.data(), parsed_frame_max_payload.size(), parsed_frame);
    runTest("parseFrame (max payload) - overall result", parse_result_max_payload);
    runTest("parseFrame (max payload) - isValid flag", parsed_frame.isValid);
    runTest("parseFrame (max payload) - Payload size", parsed_frame.payload.size() == max_payload_vec.size(),
            "Payload size mismatch", parsed_frame.payload.size(), max_payload_vec.size());
    runTest("parseFrame (max payload) - Payload content", parsed_frame.payload == max_payload_vec);
    if (!(parsed_frame.payload == max_payload_vec)) {
        printHexVector("    Expected payload", max_payload_vec);
        printHexVector("    Actual payload", parsed_frame.payload);
    }

    // Test case 21: Parse a frame with corrupted CRC
    std::vector<uint8_t> corrupted_crc_frame = parsed_frame_small_payload;
    corrupted_crc_frame[corrupted_crc_frame.size() - 1] ^= 0x01; // Corrupt last byte of CRC
    printHexVector("Frame to parse (corrupted CRC)", corrupted_crc_frame);
    bool parse_result_corrupted_crc = IoHomeNode::parseFrame(corrupted_crc_frame.data(), corrupted_crc_frame.size(), parsed_frame);
    runTest("parseFrame (corrupted CRC) - overall result", !parse_result_corrupted_crc);
    runTest("parseFrame (corrupted CRC) - isValid flag", !parsed_frame.isValid);

    // Test case 22: Parse a frame that is too short (less than min header + cmd + CRC)
    std::vector<uint8_t> too_short_frame = {0x01, 0x02, 0x03, 0x04}; // Arbitrary short data
    printHexVector("Frame to parse (too short)", too_short_frame);
    bool parse_result_too_short = IoHomeNode::parseFrame(too_short_frame.data(), too_short_frame.size(), parsed_frame);
    runTest("parseFrame (too short) - overall result", !parse_result_too_short);
    runTest("parseFrame (too short) - isValid flag", !parsed_frame.isValid);

    // Test case 23: Parse a frame where declared length doesn't match actual data length (after CRC check)
    // This creates a valid CRC but internal length is inconsistent.
    std::vector<uint8_t> inconsistent_len_frame = parsed_frame_small_payload;
    inconsistent_len_frame[IOHOME_CTRLBYTE0_POS] = (inconsistent_len_frame[IOHOME_CTRLBYTE0_POS] & ~0x1F) | (0x02 - 1); // Declares length of 2, but frame has more
    printHexVector("Frame to parse (inconsistent declared length)", inconsistent_len_frame);
    bool parse_result_inconsistent_len = IoHomeNode::parseFrame(inconsistent_len_frame.data(), inconsistent_len_frame.size(), parsed_frame);
    runTest("parseFrame (inconsistent declared length) - overall result", !parse_result_inconsistent_len);
    runTest("parseFrame (inconsistent declared length) - isValid flag", !parsed_frame.isValid);
}

void runRadioTransmitTests()
{
    IoHomeTestFixture fx; // All values initialized once here

    MockPhysicalLayer mockPhy;
    IoHomeChannel_t test_channel = {868, 95}; // e.g., 868.95 MHz
    IoHomeNode ioHomeNode_tx_test(&mockPhy, &test_channel);

    std::vector<uint8_t> no_payload_vec = {};

    // Test case 24: Transmit a valid frame (successful scenario)
    std::vector<uint8_t> frame_to_transmit = IoHomeNode::buildFrame(
      fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, no_payload_vec
    );
    printHexVector("Frame to transmit (success)", frame_to_transmit);
    mockPhy.startTransmitResult = RADIOLIB_ERR_NONE; // Simulate success
    int16_t tx_result_success = ioHomeNode_tx_test.transmitFrame(frame_to_transmit);
    runTest("transmitFrame (success) - return code", tx_result_success == RADIOLIB_ERR_NONE,
            "Transmit return code mismatch", tx_result_success, (int16_t)RADIOLIB_ERR_NONE);
    float expected_freq = (test_channel.c0 + test_channel.c1 / 100.0);
    float actual_freq_set = mockPhy.actualFrequencySet;
    float epsilon = 0.00001; // Epsilon for floating-point comparison

    runTest("transmitFrame (success) - frequency set", std::abs(actual_freq_set - expected_freq) < epsilon,
            "Frequency set mismatch", actual_freq_set, expected_freq);

    // Test case 25: Transmit a frame (simulated failure)
    mockPhy.startTransmitResult = RADIOLIB_ERR_TX_TIMEOUT; // Simulate failure
    int16_t tx_result_failure = ioHomeNode_tx_test.transmitFrame(frame_to_transmit);
    runTest("transmitFrame (failure) - return code", tx_result_failure == RADIOLIB_ERR_TX_TIMEOUT,
            "Transmit return code mismatch", tx_result_failure, (int16_t)RADIOLIB_ERR_TX_TIMEOUT);
    // Frequency should still be set correctly even if transmit fails
    runTest("transmitFrame (failure) - frequency set", std::abs(mockPhy.actualFrequencySet - expected_freq) < epsilon,
            "Frequency set mismatch", actual_freq_set, expected_freq);


    std::cout << "All IoHomeNode Transmit tests completed." << std::endl;
}

void runRadioReceiveTests() {
    IoHomeTestFixture fx; // All values initialized once here

    MockPhysicalLayer mockPhy;
    IoHomeChannel_t test_channel = {868, 95}; // e.g., 868.95 MHz
    IoHomeNode ioHomeNode_tx_test(&mockPhy, &test_channel);
    float epsilon = 0.00001; // Epsilon for floating-point comparison

    std::vector<uint8_t> no_payload_vec = {};

    std::cout << "\nStarting IoHomeNode Receive tests..." << std::endl;

    // Test case 26: Receive a valid frame successfully
    std::vector<uint8_t> valid_rx_payload = {0xDE, 0xAD, 0xBE, 0xEF};
    std::vector<uint8_t> valid_rx_frame = IoHomeNode::buildFrame(
        fx.ctrl0_base, fx.ctrl1, fx.src_mac, fx.dest_mac, fx.cmd_id, valid_rx_payload
    );
    printHexVector("Valid RX Frame (expected)", valid_rx_frame);
    mockPhy.rxBufferInternal = valid_rx_frame;
    mockPhy.mockPacketLength = valid_rx_frame.size();
    mockPhy.startReceiveResult = RADIOLIB_ERR_NONE; // Simulate radio successfully entering RX mode
    mockPhy.readDataResult = RADIOLIB_ERR_NONE;     // Simulate successful read

    IoHomeFrame_t receivedFrame1; // Declare receivedFrame1 here
        int16_t rx_result_success = ioHomeNode_tx_test.receiveFrame(receivedFrame1);
    runTest("receiveFrame (success) - return code", rx_result_success == RADIOLIB_ERR_NONE,
            "Receive return code mismatch", rx_result_success, (int16_t)RADIOLIB_ERR_NONE);
    runTest("receiveFrame (success) - frame valid", receivedFrame1.isValid);
    runTest("receiveFrame (success) - CTRL0", receivedFrame1.ctrlByte0 == valid_rx_frame[IOHOME_CTRLBYTE0_POS],
            "CTRL0 mismatch", receivedFrame1.ctrlByte0, valid_rx_frame[IOHOME_CTRLBYTE0_POS]);
    runTest("receiveFrame (success) - Payload", receivedFrame1.payload == valid_rx_payload);
    if (!(receivedFrame1.payload == valid_rx_payload)) {
        printHexVector("    Expected payload", valid_rx_payload); // Corrected variable name
        printHexVector("    Actual payload", receivedFrame1.payload); // Corrected variable name
    }
    runTest("receiveFrame (success) - frequency set", std::abs(mockPhy.actualFrequencySet - (test_channel.c0 + test_channel.c1 / 100.0)) < epsilon,
            "Frequency set mismatch", mockPhy.actualFrequencySet, (test_channel.c0 + test_channel.c1 / 100.0));

    // Test case 27: Receive frame, but CRC is corrupted (parseFrame fails)
    std::vector<uint8_t> corrupted_rx_frame = valid_rx_frame;
    corrupted_rx_frame[corrupted_rx_frame.size() - 1] ^= 0x01; // Corrupt last byte of CRC
    printHexVector("Corrupted RX Frame", corrupted_rx_frame);
    mockPhy.rxBufferInternal = corrupted_rx_frame;
    mockPhy.mockPacketLength = corrupted_rx_frame.size();
    mockPhy.startReceiveResult = RADIOLIB_ERR_NONE;
    mockPhy.readDataResult = RADIOLIB_ERR_NONE;

    IoHomeFrame_t receivedFrame2;
    int16_t rx_result_corrupted_crc = ioHomeNode_tx_test.receiveFrame(receivedFrame2);
    runTest("receiveFrame (corrupted CRC) - return code", rx_result_corrupted_crc == RADIOLIB_ERR_CRC_MISMATCH,
            "Receive return code mismatch", rx_result_corrupted_crc, (int16_t)RADIOLIB_ERR_CRC_MISMATCH);
    runTest("receiveFrame (corrupted CRC) - frame invalid", !receivedFrame2.isValid);

    // Test case 28: Radio fails to start receive mode
    mockPhy.startReceiveResult = RADIOLIB_ERR_RX_TIMEOUT; // Simulate radio error
    mockPhy.rxBufferInternal.clear(); // No data to receive
    mockPhy.mockPacketLength = 0;
    mockPhy.readDataResult = RADIOLIB_ERR_NONE;

    IoHomeFrame_t receivedFrame3;
    int16_t rx_result_radio_fail = ioHomeNode_tx_test.receiveFrame(receivedFrame3);
    runTest("receiveFrame (radio fail) - return code", rx_result_radio_fail == RADIOLIB_ERR_RX_TIMEOUT,
            "Receive return code mismatch", rx_result_radio_fail, (int16_t)RADIOLIB_ERR_RX_TIMEOUT);
    runTest("receiveFrame (radio fail) - frame invalid", !receivedFrame3.isValid);

    // Test case 29: Radio receives no packet (getPacketLength returns 0)
    mockPhy.startReceiveResult = RADIOLIB_ERR_NONE;
    mockPhy.rxBufferInternal.clear();
    mockPhy.mockPacketLength = 0; // Simulate no packet received
    mockPhy.readDataResult = RADIOLIB_ERR_NONE;

    IoHomeFrame_t receivedFrame4;
    int16_t rx_result_no_packet = ioHomeNode_tx_test.receiveFrame(receivedFrame4);
    runTest("receiveFrame (no packet) - return code", rx_result_no_packet == RADIOLIB_ERR_RX_TIMEOUT,
            "Receive return code mismatch", rx_result_no_packet, (int16_t)RADIOLIB_ERR_RX_TIMEOUT);
    runTest("receiveFrame (no packet) - frame invalid", !receivedFrame4.isValid);

    // Test case 30: ReadData fails after packet detected
    mockPhy.startReceiveResult = RADIOLIB_ERR_NONE;
    mockPhy.rxBufferInternal = valid_rx_frame;
    mockPhy.mockPacketLength = valid_rx_frame.size();
    mockPhy.readDataResult = RADIOLIB_ERR_RX_TIMEOUT; // Simulate SPI read failure

    IoHomeFrame_t receivedFrame5;
    int16_t rx_result_read_fail = ioHomeNode_tx_test.receiveFrame(receivedFrame5);
    runTest("receiveFrame (read data fail) - return code", rx_result_read_fail == RADIOLIB_ERR_RX_TIMEOUT,
            "Receive return code mismatch", rx_result_read_fail, (int16_t)RADIOLIB_ERR_RX_TIMEOUT);
    runTest("receiveFrame (read data fail) - frame invalid", !receivedFrame5.isValid);

    // Test case 31: Frame too short to be parsed as io-homecontrol (implicitly handled by parseFrame)
    std::vector<uint8_t> short_rx_frame = {0x01, 0x02}; // Much shorter than min frame len
    printHexVector("Short RX Frame", short_rx_frame);
    mockPhy.rxBufferInternal = short_rx_frame;
    mockPhy.mockPacketLength = short_rx_frame.size();
    mockPhy.startReceiveResult = RADIOLIB_ERR_NONE;
    mockPhy.readDataResult = RADIOLIB_ERR_NONE;

    IoHomeFrame_t receivedFrame6;
    int16_t rx_result_too_short = ioHomeNode_tx_test.receiveFrame(receivedFrame6);
    runTest("receiveFrame (too short) - return code", rx_result_too_short == RADIOLIB_ERR_CRC_MISMATCH, // parseFrame returns false, which is mapped to CRC_MISMATCH
            "Receive return code mismatch", rx_result_too_short, (int16_t)RADIOLIB_ERR_CRC_MISMATCH);
    runTest("receiveFrame (too short) - frame invalid", !receivedFrame6.isValid);

}

int main() {
    std::cout << "Starting IoHomeNode CRC16 tests..." << std::endl;
    runCRCTests();
    std::cout << "All IoHomeNode CRC16 tests completed." << std::endl;


    std::cout << "\nStarting IoHomeNode Frame Utility tests..." << std::endl;
    runFrameTests();
    std::cout << "All IoHomeNode Frame Parsing tests completed." << std::endl;

    std::cout << "\nStarting IoHomeNode Transmit tests..." << std::endl;
    runRadioTransmitTests();
    std::cout << "All IoHomeNode Transmit tests completed." << std::endl;

    std::cout << "\nStarting IoHomeNode Receive tests..." << std::endl;
    runRadioReceiveTests();
    std::cout << "All IoHomeNode Receive tests completed." << std::endl;

    return 0;
}
