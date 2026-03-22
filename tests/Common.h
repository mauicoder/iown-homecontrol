#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip> // For std::hex, std::setw, std::setfill
#include <cmath>   // Required for std::abs
#include <string>

// Helper function to print test results with optional debug info
template<typename T_Actual, typename T_Expected>
void runTest(const std::string& testName, bool condition, const std::string& debugMsg = "", T_Actual actualVal = T_Actual(), T_Expected expectedVal = T_Expected()) {
    std::cout << "Running test: " << testName << " - ";
    if (condition) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
        if (!debugMsg.empty()) {
            std::cout << "    DEBUG: " << debugMsg << std::endl;
        }
        // Generic debug output for different types
        if constexpr (std::is_integral<T_Actual>::value && std::is_integral<T_Expected>::value) {
            std::cout << "    Expected: 0x" << std::hex << std::setw(sizeof(T_Expected)*2) << std::setfill('0') << (long long)expectedVal
                      << ", Actual: 0x" << std::hex << std::setw(sizeof(T_Actual)*2) << std::setfill('0') << (long long)actualVal << std::endl;
            std::cout << std::dec; // Reset to decimal
        } else if constexpr (std::is_floating_point<T_Actual>::value && std::is_floating_point<T_Expected>::value) {
            std::cout << "    Expected: " << std::fixed << std::setprecision(5) << expectedVal
                      << ", Actual: " << std::fixed << std::setprecision(5) << actualVal << std::endl;
        }
    }
    assert(condition); // Will terminate if condition is false
}

// Overload for tests without specific values to compare, just a boolean condition
void runTest(const std::string& testName, bool condition) {
    std::cout << "Running test: " << testName << " - ";
    if (condition) {
        std::cout << "PASSED" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
    assert(condition); // Will terminate if condition is false
}

// Helper to print a vector of bytes in hex
void printHexVector(const std::string& label, const std::vector<uint8_t>& vec) {
    std::cout << "    " << label << " (len " << vec.size() << "): ";
    for (uint8_t byte : vec) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << std::endl;
}

#endif // COMMON_H
