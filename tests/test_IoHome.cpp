#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip> // For std::hex, std::setw, std::setfill

// Include the header for the function we want to test
#include "IoHome.h"

// Helper function to print test results
void runTest(const std::string& testName, bool condition) {
    std::cout << "Running test: " << testName << " - ";
    if (condition) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
    assert(condition); // Will terminate if condition is false
}

int main() {
    std::cout << "Starting IoHomeNode CRC16 tests..." << std::endl;

    // Test case 1: Empty data
    std::vector<uint8_t> data1 = {};
    uint16_t expected_crc1 = 0x0000;
    uint16_t actual_crc1 = IoHomeNode::crc16(data1.data(), data1.size());
    runTest("Empty data", actual_crc1 == expected_crc1);
    
    // Test case 2: Single byte 0x00
    std::vector<uint8_t> data2 = {0x00};
    uint16_t expected_crc2 = 0x0000;
    uint16_t actual_crc2 = IoHomeNode::crc16(data2.data(), data2.size());
    runTest("Single byte 0x00", actual_crc2 == expected_crc2);

    // Test case 3: Single byte 0x01
    std::vector<uint8_t> data3 = {0x01};
    uint16_t expected_crc3 = 0x1189; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc3 = IoHomeNode::crc16(data3.data(), data3.size());
    runTest("Single byte 0x01", actual_crc3 == expected_crc3);

    // Test case 4: Single byte 0x10
    std::vector<uint8_t> data4 = {0x10};
    uint16_t expected_crc4 = 0x1081; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc4 = IoHomeNode::crc16(data4.data(), data4.size());
    runTest("Single byte 0x10", actual_crc4 == expected_crc4);

    // Test case 5: "123456789" ASCII
    std::vector<uint8_t> data5 = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint16_t expected_crc5 = 0x2189; // Calculated using online CRC-16/KERMIT calculator (poly 0x1021, init 0x0000, no reverse, no xor out)
    uint16_t actual_crc5 = IoHomeNode::crc16(data5.data(), data5.size());
    runTest("ASCII '123456789'", actual_crc5 == expected_crc5);

    // Test case 6: "DEADBEEF" hex
    std::vector<uint8_t> data6 = {0xDE, 0xAD, 0xBE, 0xEF};
    uint16_t expected_crc6 = 0x1915; // Corrected expected value for CRC-16/KERMIT (poly 0x1021, init 0x0000, no reflection, no xor out)
    uint16_t actual_crc6 = IoHomeNode::crc16(data6.data(), data6.size());
    
    // Debugging output for Hex DEAD BEEF test
    if (actual_crc6 != expected_crc6) {
        std::cout << "DEBUG: For 'Hex DEAD BEEF', expected CRC: 0x" 
                  << std::hex << std::setw(4) << std::setfill('0') << expected_crc6
                  << ", actual CRC: 0x" 
                  << std::hex << std::setw(4) << std::setfill('0') << actual_crc6 << std::endl;
    }
    std::cout << std::dec; // Reset to decimal for subsequent output
    runTest("Hex DEAD BEEF", actual_crc6 == expected_crc6);

    // Test case 7: Data length 10, all zeros
    std::vector<uint8_t> data7(10, 0x00);
    uint16_t expected_crc7 = 0x0000;
    uint16_t actual_crc7 = IoHomeNode::crc16(data7.data(), data7.size());
    runTest("10 zero bytes", actual_crc7 == expected_crc7);

    // Test case 8: Data length 10, all ones (0xFF)
    std::vector<uint8_t> data8(10, 0xFF);
    uint16_t expected_crc8 = 0x1BE2; // Corrected expected value for CRC-16/KERMIT, 10 bytes of 0xFF
    uint16_t actual_crc8 = IoHomeNode::crc16(data8.data(), data8.size());
    runTest("10 0xFF bytes", actual_crc8 == expected_crc8);

    // Test case 9: Alternating bytes (0x55, 0xAA, 0x55, 0xAA)
    std::vector<uint8_t> data9 = {0x55, 0xAA, 0x55, 0xAA};
    uint16_t expected_crc9 = 0x60F3; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc9 = IoHomeNode::crc16(data9.data(), data9.size());
    runTest("Alternating 0x55/0xAA bytes", actual_crc9 == expected_crc9);

    // Test case 10: Longer string "The quick brown fox jumps over the lazy dog."
    std::string str10 = "The quick brown fox jumps over the lazy dog.";
    std::vector<uint8_t> data10(str10.begin(), str10.end());
    uint16_t expected_crc10 = 0x07FC; // Corrected expected value for CRC-16/KERMIT
    uint16_t actual_crc10 = IoHomeNode::crc16(data10.data(), data10.size());
    runTest("Longer string 'The quick brown fox...'", actual_crc10 == expected_crc10);

    std::cout << "\nStarting IoHomeNode Frame Utility tests..." << std::endl;

    // --- Test IOHOME_MSG_LEN macro ---
    // A message with length 0x05 (0b00101) in CTRLBYTE0 would mean 5 + 1 = 6 bytes message length
    // Frame: [CTRLBYTE0 (0x05), CTRLBYTE1, MAC_SRC, MAC_DEST, COMMAND, PARAMETER]
    std::vector<uint8_t> dummy_frame_len_5 = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
    size_t expected_msg_len_5 = (0x05 & 0x1F) + 1; // Expected: 6
    runTest("IOHOME_MSG_LEN (length 5)", IOHOME_MSG_LEN(dummy_frame_len_5.data()) == expected_msg_len_5);

    // A message with length 0x1F (0b11111) in CTRLBYTE0 would mean 31 + 1 = 32 bytes message length
    std::vector<uint8_t> dummy_frame_len_1F = {0x1F}; // Max length code
    size_t expected_msg_len_1F = (0x1F & 0x1F) + 1; // Expected: 32
    runTest("IOHOME_MSG_LEN (length 1F)", IOHOME_MSG_LEN(dummy_frame_len_1F.data()) == expected_msg_len_1F);

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


    std::cout << "All IoHomeNode CRC16 tests completed." << std::endl;

    return 0;
}
