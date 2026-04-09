# Here is a collection of packets valid or invalid received from the heltec lora 32 v3

Some packets are received when the rssi is way too low for a valid signal. These packets are usually garbage.


 *  Executing task: platformio device monitor --environment heltec_wifi_lora_32_V3

--- Terminal on /dev/cu.usbserial-0001 | 115200 8-N-1
--- Available filters and text transformations: debug, default, direct, esp32_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at https://bit.ly/pio-monitor-filters
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
RSSI: -119.50 dBm | STATUS: OK
RSSI: -117.50 dBm | STATUS: OK
RSSI: -111.50 dBm | STATUS: OK
[RAW] Received packet! RSSI: -117.50 | Length: 255
      Data: 34 C0 15 25 0D 4B 54 33 D4 95 10 54 10 34 01 00 40 10 04 01 00 50 35 1C 87 40 40 10 05 23 14 D5 44 3B 95 11 73 48 26 53 45 65 15 63 7E 0D AC 99 10 81 EA 83 75 B3 50 7F 3E 46 53 A6 45 3F F2 AE EE 4D 79 B7 E2 69 57 58 A6 02 8F 02 29 00 B9 CC 86 6F 09 F9 EE D5 1B 9F 8D ED 30 BC 55 40 FB D7 A8 4B 9A C8 5A E1 1B 09 20 1A 94 74 99 2A 2F A6 D2 86 97 74 11 0D A5 03 7C 86 37 D6 A9 EF B8 3D 41 B2 74 4F 98 87 E2 8D 9E 8A 92 EA 86 68 D6 56 ED 2E C2 DD F3 5B 39 9E 52 4A 0F EC 7B 3B 7B 85 A3 4D A0 B4 7A D9 31 E5 D1 CF CB A6 B3 AC C6 DD 8B 44 B9 81 D1 9D DC 2C E7 9B C0 4F 62 86 27 6C 44 98 02 2F B2 A9 DB F8 A7 24 4A 57 C8 A9 55 7C 86 B7 32 60 C0 3B FD 0B 3E BF FF 21 15 6D A4 7F C8 CD 51 26 46 3D 2E 7E 01 72 10 AA 95 C9 A6 E8 B9 00 8F 4E D3 76 9B 68 2A 31 3F EA A9 25 88
[RAW] Discarded noise/ghost packet during UART extraction.
RSSI: -113.00 dBm | STATUS: OK
[RAW] Received packet! RSSI: -117.50 | Length: 255
      Data: 34 C0 15 25 0D 4B 47 75 1C 43 10 54 10 34 01 10 40 10 04 01 00 54 84 35 2D 40 40 10 05 FD 21 55 5C 8E 44 FD 61 25 1C 2F 5D E2 51 A9 69 9C 1D BD 46 6E 36 6C E6 59 AC 3D 4D 5B E9 30 20 21 E8 58 D3 E6 21 07 2E 21 7C BC 26 2A 20 FF ED CD 6A 79 27 D4 E2 40 D5 E1 EB B0 02 BE C1 8B 30 B5 78 A9 59 13 81 B5 7D 06 E4 07 FD 0E 75 7F 79 24 6E 1E 86 27 B7 35 A4 65 81 BD BE D1 ED B5 94 7D 5B 8C 80 CB 02 57 9D 2B 83 7D B3 8A 6C FF 5A 70 1E 24 EA D5 AB 20 D2 D9 7F BB 67 AE 0B 74 53 D2 A3 88 C8 40 5D 01 A8 78 34 8F 07 5F C5 10 44 97 F1 B8 EF 9C A9 1F 5D E4 47 D6 A0 A7 FE 20 85 78 D1 54 A3 82 BC 1F A7 A4 87 EF FA A8 D4 6C EB 2D E2 A9 D7 FD 08 8F 7B DE A5 55 CA 67 C1 3F AA 6B 08 C3 6C 0C 55 BA A2 2C E2 93 D0 85 6B E5 45 D4 CA CD 71 E3 F7 B9 84 15 57 FE F9 85 A0 FD 80 8E E0
[RAW] Discarded noise/ghost packet during UART extraction.
RSSI: -114.00 dBm | STATUS: OK
RSSI: -116.50 dBm | STATUS: OK
RSSI: -113.00 dBm | STATUS: OK
RSSI: -101.00 dBm | STATUS: OK
RSSI: -121.50 dBm | STATUS: OK
RSSI: -114.50 dBm | STATUS: OK
RSSI: -117.50 dBm | STATUS: OK
RSSI: -114.00 dBm | STATUS: OK
RSSI: -125.50 dBm | STATUS: OK
RSSI: -114.00 dBm | STATUS: OK
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce2820,len:0x10cc
load:0x403c8700,len:0xc2c
load:0x403cb700,len:0x30c0
entry 0x403c88b8
   HELTEC V3.2 IoHome NODE
===============================
Initializing Radio... SUCCESS
Setting packet parameters... SUCCESS
Setting sync word... SUCCESS (0x57FD99)
Radio is LISTENING.
Initializing IoHomeNode... SUCCESS
RSSI: -112.00 dBm | STATUS: OK
RSSI: -122.50 dBm | STATUS: OK
RSSI: -113.50 dBm | STATUS: OK
RSSI: -119.50 dBm | STATUS: OK
RSSI: -115.00 dBm | STATUS: OK
RSSI: -121.50 dBm | STATUS: OK
RSSI: -117.00 dBm | STATUS: OK
[RAW] Received packet! RSSI: -19.50 | Length: 255
      Data: 0F C0 10 05 01 7E CB F7 6C 11 00 50 16 14 01 00 40 36 5C 01 00 46 13 FD C5 7F 40 D4 95 47 06 57 F7 FF FF FF FF FF FF FF FC 00 00 00 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AF FB 32 1F 80 20 0A 02 FD 97 EE D8 22 00 A0 2C 28 02 00 80 6C B8 02 00 8C 27 FB 8A FE 81 A9 2A 8E 0C AF EF FF FF 80 03 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AB FE CC 87 E0 08 02 80 BF 65 FB
[IOHOME] Decoded: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 D3 00 00 0C FE 47 7F 60 49 C5 30 FD FF
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 D3 00 00 0C FE 47 7F 60 49 C5 30 FD FF
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xFFFD, Received CRC: 0xFFFD
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 0001BF, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0CFE, MAC (first 6 bytes): 47 7F 60 49 C5 30
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x47, Expected 0x77
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xFFFD, Received CRC: 0xFFFD
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #1 (CRC OK) <<<
Command: 0x00 | Source: 0001BF | Dest: FAB710
>>> AES PARSE FAILED (Expected until AES keys are provided) <<<
RSSI: -127.50 dBm | STATUS: OK
[RAW] Received packet! RSSI: -20.00 | Length: 255
      Data: 37 C0 90 04 01 7E 4B F7 6C 11 00 50 16 14 01 00 40 10 04 61 7F D0 B5 BC 1D 49 5A 72 24 C9 4B FF F8 00 00 1F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 5F F6 64 DF 00 40 10 05 F9 2F DD B0 44 01 40 58 50 04 01 00 40 11 85 FF 42 D6 F0 75 25 69 C8 96 1D AD FF FF E0 00 00 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 FF 66 4D F0 04 01 00 5F 92 FD DB 04 40 14 05 85 00 40 10
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
[RAW] Received packet! RSSI: -20.00 | Length: 255
      Data: C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 40 11 84 01 00 56 10 04 15 04 D9 57 35 E5 20 CF D3 0F FF 80 00 00 07 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FD 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 57 FD 99 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 40 11 84 01 00 56 10 04 15 04 D9 57 35 E5 20 CF D3 0F FF 00 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA FF B3 21 F8 02 00 80 2F C9 7E ED 82 20 49 02 FF 10
[RAW] Discarded noise/ghost packet during UART extraction.
[RAW] Received packet! RSSI: -20.00 | Length: 255
      Data: 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 40 15 05 FF 00 56 14 05 E3 05 40 30 B4 63 29 DC F2 D7 FF FF 00 00 7F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 7F D9 90 FC 01 00 40 17 E4 BF 76 C1 10 24 81 7F D0 16 14 01 50 5F F0 05 61 40 5E 30 54 03 0B 46 32 9D CF 2D 7F FC 00 00 00 00 00 00 00 00 00 06 F5 28 A6 7B 97 0D E3 87 B2 82 B6 54 35 FF FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA
[IOHOME] Decoded: F8 00 00 00 3F FA B7 10 20 02 FF 01 43 00 05 FF 00 0D 01 8F 50 80 68 8C CA E7 5A
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 00 3F FA B7 10 20 02 FF 01 43 00 05 FF 00 0D 01 8F 50 80 68 8C CA E7 5A
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x5AE7, Received CRC: 0x5AE7
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 00003F, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x20
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0D01, MAC (first 6 bytes): 8F 50 80 68 8C CA
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x8F, Expected 0xDD
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x5AE7, Received CRC: 0x5AE7
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #2 (CRC OK) <<<
Command: 0x20 | Source: 00003F | Dest: FAB710
>>> AES PARSE FAILED (Expected until AES keys are provided) <<<
RSSI: -114.50 dBm | STATUS: OK
RSSI: -111.50 dBm | STATUS: OK
RSSI: -118.00 dBm | STATUS: OK
RSSI: -113.50 dBm | STATUS: OK
RSSI: -112.00 dBm | STATUS: OK
[RAW] Received packet! RSSI: -18.00 | Length: 255
      Data: 0F C0 10 05 01 7E CB F7 6C 11 00 50 16 14 27 00 40 36 5C 01 00 56 16 05 8D 27 5C 90 CD 19 6D 4E 73 57 FF FF FF FF FF FF FF FF FF F8 00 00 00 00 1F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AF FB 32 1F 80 20 0A 02 FD 97 EE D8 22 00 A0 2C 28 4E 00 80 6C B8 02 00 AC 2C 0B 1A 4E B9 21 9A 32 DA 9C E6 AF FF 00 00 00 7F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 7F D9 90 FC 01 00
[IOHOME] Decoded: F8 00 00 01 BF FA B7 10 00 01 43 C8 00 80 D3 00 00 0D 03 63 72 27 98 31 5B CE 56
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 01 BF FA B7 10 00 01 43 C8 00 80 D3 00 00 0D 03 63 72 27 98 31 5B CE 56
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x56CE, Received CRC: 0x56CE
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 0001BF, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0D03, MAC (first 6 bytes): 63 72 27 98 31 5B
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x63, Expected 0xA3
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x56CE, Received CRC: 0x56CE
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #3 (CRC OK) <<<
Command: 0x00 | Source: 0001BF | Dest: FAB710
>>> AES PARSE FAILED (Expected until AES keys are provided) <<<
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: 37 C0 90 04 01 7E 4B F7 6C 11 00 50 16 14 27 00 40 10 05 61 10 40 73 15 D7 16 56 F7 EC 03 33 FF FC 00 00 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 5F F6 64 DF 00 40 10 05 F9 2F DD B0 44 01 40 58 50 9C 01 00 40 15 84 41 01 CC 57 5C 59 5B DF B5 34 4D FF FF 80 00 00 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 7F D9 93 7C 01 00 40 17 E4 BF 76 C1 10 05 01 61 42 70
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
[RAW] Received packet! RSSI: -18.50 | Length: 255
      Data: C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 50 11 84 01 00 56 15 04 FF 63 4F D7 3C 81 62 5E 92 0F FF F8 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF EA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA BF EC C8 7E 00 80 20 0B F2 5F BB 60 88 12 40 BF E8 0B 0A 80 8C 20 08 02 B0 A8 27 FB 1A 7E B9 E4 0B 12 F4 90 7F FF C0 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF EA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA BF EC C8 7E 00 80 20 0B F2 5F BB 60 88 12 40 BF E8 0B 70
[RAW] Discarded noise/ghost packet during UART extraction.
RSSI: -19.00 dBm | STATUS: OK
[RAW] Received packet! RSSI: -18.50 | Length: 255
      Data: 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 50 15 05 FF 00 56 13 04 C9 34 4C D0 CD 85 0F CD D7 B7 FF 80 00 00 7F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 7F D9 90 FC 01 00 40 17 E4 BF 76 C1 10 24 81 7F D0 16 15 01 50 5F F0 05 61 30 4C 93 44 CD 0C D8 50 FC DD 7B 7F F8 00 00 00 00 00 00 00 00 00 09 42 C1 A8 37 3A E6 D4 DA 68 63 DA 84 E7 BF CA FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
RSSI: -122.50 dBm | STATUS: OK
RSSI: -117.50 dBm | STATUS: OK
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: 37 C0 10 04 01 7E 4B F7 6C 11 00 50 16 14 97 00 40 10 05 61 08 4D D2 24 07 41 5A 90 F5 DD 7C FF FF FF 80 00 00 00 7F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 7F D9 93 7C 01 00 40 17 E4 BF 76 C1 10 05 01 61 49 70 04 01 00 56 10 84 DD 22 40 74 15 A9 0F 5D D7 CF FF C0 01 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 FF 66 4D F0 04 01 00 5F 92 FD DB 04 40 14 05 85 25 C0 10 04
[IOHOME] Decoded: F6 00 00 00 3F FA B7 10 00 01 43 D2 00 00 00 0D 08 76 22 C0 41 2B 78 77 9F
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 25 ---
[IoHomeNode::parseFrame] Raw bytes: F6 00 00 00 3F FA B7 10 00 01 43 D2 00 00 00 0D 08 76 22 C0 41 2B 78 77 9F
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 25
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x9F77, Received CRC: 0x9F77
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF6 (Binary: 11110110), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 00003F, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 23 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0D08, MAC (first 6 bytes): 76 22 C0 41 2B 78
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x76, Expected 0x4F
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 25
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x9F77, Received CRC: 0x9F77
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #4 (CRC OK) <<<
Command: 0x00 | Source: 00003F | Dest: FAB710
>>> AES PARSE FAILED (Expected until AES keys are provided) <<<
RSSI: -19.00 dBm | STATUS: OK
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: 0F C0 90 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 48 11 84 01 00 56 14 84 D5 18 CB 91 05 F3 2B CD 31 47 FF C0 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA FF B3 21 F8 02 00 80 2F C9 7E ED 82 20 49 02 FF A0 2C 29 02 30 80 20 0A C2 90 9A A3 19 72 20 BE 65 7A D6 24 FF F0 00 00 07 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FD 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 57 FD 99 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 48 15 05 FF 00 56 12 84 3D 6E C5 13 A5 33 4D 51 17 CF FF FF 00 00 00 00 03 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FD 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 57 FD 99 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 48 15 05 FF 00 56 12 84 3D 6E C5 13 A5 33 4D 51 17 CF FF 80 00 00 7F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 7F D9 90 FC 01 00 40 17 E4 BF 76 C1 10 24 01
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
[RAW] Received packet! RSSI: -75.50 | Length: 255
      Data: 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 48 15 05 FF 00 56 12 84 3D 6E C5 13 A5 33 4D 51 17 CF FF 80 00 00 00 00 00 00 00 00 00 DE B6 B0 D7 E0 9F 39 CF 21 92 A9 BF 5D 18 96 3A 77 CB 58 65 A2 34 4E 12 6F 40 EE 46 DC AF 5F 88 A3 3B BD 6F DC F0 99 39 0F E7 D1 FF 24 F1 EA 7A 4F 5E 5C B8 0D 46 72 CC 64 ED C8 F7 4A 37 9A 79 58 8D B1 FA D9 FD 01 08 77 5D 0F 3C E9 2D 00 E3 7E EE 55 23 BA 11 4D 4B B9 62 95 57 95 7C D7 ED EA D1 82 71 FB EC 7F A1 48 2A CD BB 7C 4D CC FF 48 B7 BB A7 97 F6 38 90 0F C2 63 E8 24 71 1F 29 1B 6C 31 52 46 2B FE A7 3E 95 54 DD BA 7D 61 F4 DC 97 C6 B5 8B FD 47 BC A9 4F 85 23 4C 47 E8 9B 05 60 8D EF B5 12 17 48 A1 A7 13 1D 5C 2B 95 F2 43 D4 2D 96 DB 55 7E 52 CE 3F A5 75 04 4E DD 78 D7 47 FB BE 2C EF DD 55 01 A6 79 E9 DF DC 92 DD 79 E8
[RAW] Discarded noise/ghost packet during UART extraction.
RSSI: -108.00 dBm | STATUS: OK
RSSI: -118.50 dBm | STATUS: OK
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: 0F C0 10 05 01 7E CB F7 6C 11 00 50 16 14 01 00 40 36 5C 01 00 56 16 85 65 68 41 51 54 3D 7E C6 B0 9F FF FF FF FF FF FF FF FF FF E0 00 00 00 00 1F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 5F F6 64 3F 00 40 14 05 FB 2F DD B0 44 01 40 58 50 04 01 00 D9 70 04 01 58 5A 15 95 A1 05 45 50 F5 FB 1A C2 7F FF 00 00 01 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 FF 66 43 F0 04 01
[IOHOME] Decoded: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 D3 00 00 0D 0B 4D 0B 50 54 78 BF AC C8
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 D3 00 00 0D 0B 4D 0B 50 54 78 BF AC C8
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xC8AC, Received CRC: 0xC8AC
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 0001BF, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0D0B, MAC (first 6 bytes): 4D 0B 50 54 78 BF
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x4D, Expected 0xCF
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xC8AC, Received CRC: 0xC8AC
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #5 (CRC OK) <<<
Command: 0x00 | Source: 0001BF | Dest: FAB710
>>> AES PARSE FAILED (Expected until AES keys are provided) <<<
[RAW] Received packet! RSSI: -18.50 | Length: 255
      Data: 37 C0 90 04 01 7E 4B F7 6C 11 00 50 16 14 01 00 40 10 05 61 18 5E 35 4C 33 18 46 94 14 89 7E 7F FC 00 00 1F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 5F F6 64 DF 00 40 10 05 F9 2F DD B0 44 01 40 58 50 04 01 00 40 15 84 61 78 D5 30 CC 61 1A 50 57 1D 7B FF FF FF 80 00 00 03 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AB FE CC 9B E0 08 02 00 BF 25 FB B6 08 80 28 0B 0A 00 80 20
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
RSSI: -127.50 dBm | STATUS: OK
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 40 11 84 01 00 56 15 85 19 5E DA 36 15 F3 6C 5C 71 C7 FF FF 00 00 00 00 07 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AF FB 32 1F 80 20 08 02 FC 97 EE D8 22 04 90 2F FA 02 C2 80 23 08 02 00 AC 2B 0A 32 BD B4 6C 2B E6 D8 B8 E3 8F FF 00 00 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF EA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA BF EC C8 7E 00 80 20 0B F2 5F BB 60 88 12 20
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 40 15 05 FF 00 56 13 84 6B 3D DD 33 7D 85 3C 4A 75 7F FF F8 00 00 00 03 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AB FE CC 87 E0 08 02 00 BF 25 FB B6 08 81 24 0B FE 80 B0 A0 0A 82 FF 80 2B 09 C2 35 9E EE 99 BE C2 9E 25 3A BF FF E0 00 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF EA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA BF EC C8 7E 00 80 20 0B F2 5F BB 60 88 12 40
[IOHOME] Decoded: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 24 ---
[IoHomeNode::parseFrame] Raw bytes: 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 24
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xDAE3, Received CRC: 0x5555
>>> PARSE FAILED (Invalid CRC) <<<
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: 0F C0 10 05 01 7E CB F7 6C 11 00 50 16 14 01 00 40 30 9C 01 00 56 17 85 5F 34 41 30 FC 61 09 D2 B3 EF FF FF 00 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF EA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA BF EC C8 7E 00 80 28 0B F6 5F BB 60 88 02 80 B0 A0 08 02 01 84 E0 08 02 B0 BC 2A F9 A2 09 87 E3 08 4E 95 9F 7F FC 00 00 03 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AB FE CC 87 E0 08 02 80 BF 65 FB B6 08 80 28 0B 0A
[IOHOME] Decoded: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 C8 00 00 0D 0F F5 16 90 F8 0C C8 A9 BE
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 C8 00 00 0D 0F F5 16 90 F8 0C C8 A9 BE
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xBEA9, Received CRC: 0xBEA9
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 0001BF, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0D0F, MAC (first 6 bytes): F5 16 90 F8 0C C8
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0xF5, Expected 0x78
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xBEA9, Received CRC: 0xBEA9
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #6 (CRC OK) <<<
Command: 0x00 | Source: 0001BF | Dest: FAB710
>>> AES PARSE FAILED (Expected until AES keys are provided) <<<
RSSI: -118.00 dBm | STATUS: OK
RSSI: -120.00 dBm | STATUS: OK
RSSI: -115.00 dBm | STATUS: OK
RSSI: -115.00 dBm | STATUS: OK
RSSI: -115.50 dBm | STATUS: OK
RSSI: -112.00 dBm | STATUS: OK
RSSI: -122.00 dBm | STATUS: OK
 ....
 [RAW] Received packet! RSSI: -85.50 | Length: 255
      Data: 0F C0 10 04 01 7E 5E 71 84 95 30 C0 15 05 01 25 C7 94 04 01 40 51 16 9D 8D 0E 45 91 D5 E5 5B CE B3 FD E1 FF FF FF EF 2C 79 6D 91 86 39 CD 4D AA 76 39 69 95 66 2F E6 71 76 67 0E F3 8F FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 FF 66 43 F0 04 01 00 5F 97 9C 61 25 4C 30 05 41 40 49 71 E5 01 00 50 14 45 A7 63 43 91 64 75 79 56 F3 AC FF 78 7F FF FF FD 66 08 8D 77 38 DE F3 32 F5 AE 2D AA 45 05 E1 46 D0 50 B1 4F FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55
[IOHOME] Decoded: F8 00 00 00 3F CF 0C 52 86 00 05 01 D2 3C 01 00 01 11 CB 63 38 34 5C 4F ED AE FE
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 00 3F CF 0C 52 86 00 05 01 D2 3C 01 00 01 11 CB 63 38 34 5C 4F ED AE FE
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xFEAE, Received CRC: 0xFEAE
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 00003F, Dest MAC: CF0C52
[IoHomeNode::parseFrame] Command ID: 0x86
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x11CB, MAC (first 6 bytes): 63 38 34 5C 4F ED
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x63, Expected 0x0B
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xFEAE, Received CRC: 0xFEAE
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #10 (CRC OK) <<<
Command: 0x86 | Source: 00003F | Dest: CF0C52
>>> AES PARSE FAILED (Expected until AES keys are provided) <<<
RSSI: -127.50 dBm | STATUS: OK
[RAW] Received packet! RSSI: -115.50 | Length: 255
      Data: C0 10 04 01 7E 5E 71 84 95 30 C0 15 05 01 25 C7 94 04 01 40 51 16 9D 8D 0E 45 91 D5 E5 5B CE B3 FD E1 FF FF FF 4A 93 1F 14 6A D1 14 0C 11 BB 45 A7 11 E8 A0 AF 3D 13 53 FF 8B 94 21 00 DF FB 51 9E 78 4E D2 FF A1 92 35 F7 16 B0 40 70 F6 F4 82 2A C3 93 34 9E 4B 4D 74 85 15 C2 46 16 97 07 1D 6E 1C FD DF FA 16 7F 5C F7 EB 7D 07 9F 8A 75 44 B2 6E C4 31 74 5D 96 8E 92 FA 39 61 49 A3 F9 9B 1D 2F AB 92 D3 AA 97 0F 8E 26 AB FB 63 44 D2 5F 93 99 DE F2 C9 6B 9F 15 2E 69 76 6A C8 D5 3B 56 96 10 EE 3B 24 F5 E6 65 53 1A 19 99 D6 96 9E 7D F4 99 6F 5A 93 58 95 00 5A 44 9D 3B ED D4 F1 73 3B 09 FD 20 06 BF 44 9F 63 14 CB F4 A8 FA 1D CC D0 50 C9 E1 25 A1 F5 C6 F4 82 5F 8C 67 99 EA 60 A8 EF 66 FB CE 94 36 D5 59 4B 68 A4 33 00 A2 D4 CD 3C 40 F3 8B A7 5B 8C AE 82 70 91 55 9D 55
[RAW] Discarded noise/ghost packet during UART extraction.
...
