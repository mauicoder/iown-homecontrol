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
    uint16_t expected_crc3 = 0x1021;
    uint16_t actual_crc3 = IoHomeNode::crc16(data3.data(), data3.size());
    runTest("Single byte 0x01", actual_crc3 == expected_crc3);

    // Test case 4: Single byte 0x10
    std::vector<uint8_t> data4 = {0x10};
    uint16_t expected_crc4 = 0x8108;
    uint16_t actual_crc4 = IoHomeNode::crc16(data4.data(), data4.size());
    runTest("Single byte 0x10", actual_crc4 == expected_crc4);

    // Test case 5: "123456789" ASCII
    std::vector<uint8_t> data5 = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint16_t expected_crc5 = 0x2189; // Calculated using online CRC-16/KERMIT calculator (poly 0x1021, init 0x0000, no reverse, no xor out)
    uint16_t actual_crc5 = IoHomeNode::crc16(data5.data(), data5.size());
    runTest("ASCII '123456789'", actual_crc5 == expected_crc5);

    // Test case 6: "DEADBEEF" hex
    std::vector<uint8_t> data6 = {0xDE, 0xAD, 0xBE, 0xEF};
    uint16_t expected_crc6 = 0x7C26; // Calculated using online CRC-16/KERMIT calculator (poly 0x1021, init 0x0000, no reverse, no xor out)
    uint16_t actual_crc6 = IoHomeNode::crc16(data6.data(), data6.size());
    runTest("Hex DEAD BEEF", actual_crc6 == expected_crc6);

    // Test case 7: Data length 10, all zeros
    std::vector<uint8_t> data7(10, 0x00);
    uint16_t expected_crc7 = 0x0000;
    uint16_t actual_crc7 = IoHomeNode::crc16(data7.data(), data7.size());
    runTest("10 zero bytes", actual_crc7 == expected_crc7);

    // Test case 8: Data length 10, all ones (0xFF)
    std::vector<uint8_t> data8(10, 0xFF);
    uint16_t expected_crc8 = 0x5DBC; // Calculated with CRC-16/KERMIT, 10 bytes of 0xFF
    uint16_t actual_crc8 = IoHomeNode::crc16(data8.data(), data8.size());
    runTest("10 0xFF bytes", actual_crc8 == expected_crc8);


    std::cout << "All IoHomeNode CRC16 tests completed." << std::endl;

    return 0;
}
