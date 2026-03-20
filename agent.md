# `iown-homecontrol` - AI Agent Context File

This document serves as the primary context reference for AI assistants and agents working on the `iown-homecontrol` repository.

## Project Overview
The `iown-homecontrol` project aims to reverse-engineer, document, and implement the proprietary **io-homecontrol (iohc)** wireless protocol used by smart home manufacturers like Somfy and Velux.
The end goal is to create open-source libraries and hardware alternatives (like a custom LoRa32-based TaHoma gateway) to control these devices locally without relying on cloud services.

## Protocol Quick Reference
*   **Frequencies (868 MHz Band)**:
    *   Channel 1: 868.25 MHz (2W)
    *   Channel 2: 868.95 MHz (1W/2W)
    *   Channel 3: 869.85 MHz (2W)
*   **Modulation**: FSK with 19.2 kHz deviation
*   **Encoding**: NRZ, UART 8N1
*   **Data/Baud Rate**: 38400 bps
*   **Frequency Hopping (FHSS)**: 2.7ms (Patent: 3ms) per channel
*   **Modes**: 1W (One-Way / Uni-Directional), 2W (Two-Way / Bi-Directional)

## Target Hardware Stack
The primary target hardware for library implementation comprises ESP32-based boards paired with LoRa-capable radio modules (that support FSK modulation in the 868 MHz band).
*   **HelTec**: WiFi LoRa32 (v3), Wireless Bridge, Wireless Tracker, Wireless Stick / Stick Lite
*   **LilyGo**: LoRa32, T-Beam, T3-S3, T-Watch S3
*   **Adafruit**: ESP32 Feather + RFM69HCW / RFM95W

## Current Status & Missing Parts (Agent Action Items)
When assisting with this project, prioritize the following missing components, WIPs, and open tasks:

### 1. Protocol Documentation & Reverse Engineering
*   **Layer 4+**: Information and documentation are still needed for **EMS2 Frames** and **CarrierSense**.
*   **Firmware**:
    *   Actuator firmware needs to be sourced and reverse-engineered.
    *   **Target**: Hack the ESP32-based Somfy Connectivity Box.

### 2. Software & Library Implementation (WIP)
*   **C++ Libraries**:
    *   Complete the 1W (One-Way) Library implementation.
    *   Complete the 2W (Two-Way) Library implementation.
*   **MicroPython**: Build a simple MicroPython implementation for rapid testing and prototyping.
*   **rtl_433**: Provide corrections and improvements to the `rtl_433` io-homecontrol C decoder (`src/devices/somfy_iohc.c`).
*   **Kaitai Struct**: Finish Kaitai Struct implementations for easier portability (currently at 90%).

### 3. Advanced Integrations (Gateway Building)
*   Build a better/cheaper Somfy TaHoma alternative using an ESP32/LoRa32 board.
*   **Features to implement**:
    *   Add support for the older RTS (433 MHz) protocol.
    *   Expose the custom gateway as a ZigBee device for HomeAssistant integration.
    *   Expose the custom gateway as a HomeKit device (potentially using HomeSpan), including QR code generation to ease installation.

## Agent Interaction Guidelines
*   **Language & Framework**: Assume C++ (Arduino framework/PlatformIO) for main embedded implementation, and Python for scripts/crypto reverse-engineering, unless specified otherwise.
*   **Cryptography**: Be prepared to handle AES-128 (ECB/CBC) decryption/encryption routines related to stack keys, transfer keys, and initial vectors (IVs).
*   **Formatting**: Output C++ code following the Google C++ Style Guide and formatting rules defined in the repo's `.clang-format`. Output Python code adhering to PEP8.
*   **Reference**: Always consult `docs/radio.md`, `docs/linklayer.md`, and `docs/commands.md` when resolving frame formatting, MAC calculations, or command parameter bugs.
