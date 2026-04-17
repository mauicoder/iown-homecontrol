# iown-homecontrol (ESP32 Situo 5 Emulator)

This project is an **ESP32-based emulator for the Somfy Situo 5 io-homecontrol remote**. It allows you to control your io-homecontrol compatible devices (like awnings, blinds, and roller shutters) directly from your local network, featuring built-in Web and MQTT interfaces for seamless integration into Smart Home systems like **Home Assistant**.

> **Acknowledgments:** This software implementation is heavily based on the incredible reverse-engineering work of the io-homecontrol protocol. A huge thanks to **Velocet** for the big work performed on the documentation and experimentation in the parent repository: [Velocet/iown-homecontrol](https://github.com/Velocet/iown-homecontrol).
>
> Kudos to all the persons that contributed to the progress of the project!

## Features

* **Situo 5 Emulation**: Supports up to 5 independent channels/profiles.
* **Auto-Learning**: Automatically extracts AES stack keys via 1-Way Key Transfer and learns device MAC addresses on the fly.
* **Web Interface**: Built-in HTTP server for live packet sniffing, device naming, testing commands, and MQTT configuration.
* **Home Assistant Integration**: Full MQTT Auto-Discovery support. Devices automatically appear in Home Assistant as Cover entities linked to a Gateway device.
* **BLE Provisioning**: Easy Wi-Fi setup via smartphone app without hardcoding credentials.

## Supported Hardware

The project targets ESP32 boards equipped with a LoRa radio module capable of FSK modulation in the 868 MHz band.

**Tested and Verified Boards:**
* **Heltec WiFi LoRa 32 (V3.1)** (with SX1262 on 868 MHz)
* **LilyGo LoRa32** (with SX1276 on 868 MHz)

*Note: Other ESP32 + LoRa radio modules should be supported without guarantee via the underlying RadioLib library, but may require custom pin mapping adjustments.*

## Quick Setup

1. **Flash the board**: Compile and upload the firmware to your ESP32 board.
2. **Wi-Fi Provisioning**: Use the **ESP BLE Prov** smartphone app to configure the Wi-Fi credentials so the ESP32 can join your local network.
3. **Network Configuration**: Once online, optionally access the built-in web interface on port `80` (via the ESP32's assigned IP address) to configure your MQTT server connection.
4. **Clone Remote Keys**:
   * For each channel you want to simulate, press the **PROG** (PRD) button on the back of your physical remote.
   * Check the web interface (or the serial console) to verify if the encryption key was successfully received and extracted.
   * Once the key is received, press one of the standard action buttons (e.g., UP or DOWN) on your physical remote. The ESP32 will sniff this command to automatically capture and learn the MAC address of the target device.
5. **Device Naming & MQTT Discovery**:
   * In the web interface, you can assign friendly names to your discovered devices (e.g., "Left Awning").
   * These names are used when advertising the devices to your MQTT server.
   * Through MQTT Auto-Discovery, each awning will be announced to Home Assistant as a single, separated cover device with `open`, `close`, and `stop` commands available.
   * All cloned devices will be logically connected and grouped under the main gateway parent device (`esp32-iown-homecontrol`).
