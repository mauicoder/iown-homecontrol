# Here is a collection of packets valid or invalid received from the heltec lora 32 v3

## usefull link
https://github.com/merbanan/rtl_433/issues/1376
````
In my opinion you have 27 bytes with Sync, Length, address, command, 7 bytes Rolling code and 2 bytes CRC-16 Kermit Polynom: 0x1021, Init 0x0000, Final XOR: 0x0000 at the end.
```

## Currently implemented
we have frequency hopping in place and we parce with a full state machine to avoid loosing packets inbetween of 255 bytes received from the radio module
The decoding of the MAC (AES part) is still broken

## Latest Packets received

[RAW] Received packet! RSSI: -22.00 | Length: 255
      Data: 0F C0 10 05 01 7E CB F7 6C 11 00 50 16 14 01 00 40 36 5C 01 00 56 12 1C B7 27 58 F7 BD 1F 12 50 F4 17 FF FF C0 00 03 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FE AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AB FE CC 87 E0 08 02 80 BF 65 FB B6 08 80 28 0B 0A 00 80 20 1B 2E 00 80 2B 09 0E 5B 93 AC 7B DE 8F 89 28 7A 0B FF E0 00 00 1F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 5F F6 64 3F 00 40 14 05 FB 2F DD B0 44 01 40 58
>>> Searching rolling buffer (Current length: 171 bytes)
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x41E1, Received CRC: 0x41E1
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 D3 00 00 0D C2 DA 72 E3 EF F1 24 E1 41
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x41E1, Received CRC: 0x41E1
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 0001BF, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0DC2, MAC (first 6 bytes): DA 72 E3 EF F1 24
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0xDA, Expected 0x2E
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #1 (CRC OK) <<<
    Command: 0x00 | Source: 0001BF | Dest: FAB710
    >>> AES MAC VERIFICATION FAILED (Or No Keys) <<<
>>> Searching rolling buffer (Current length: 64 bytes)
Freq: 868.95 MHz | RSSI: -127.50 dBm | STATUS: OK
[RAW] Received packet! RSSI: -19.50 | Length: 255
      Data: 37 C0 90 04 01 7E 4B F7 6C 11 00 50 16 14 01 00 40 10 05 61 61 D9 71 F4 31 57 C7 F3 AC 75 44 7F FC 00 00 0F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AF FB 32 6F 80 20 08 02 FC 97 EE D8 22 00 A0 2C 28 02 00 80 20 0A C2 C3 B2 E3 E8 62 AF 8F E7 5A 76 C9 FF FF E0 00 00 00 00 3F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 5F F6 64 DF 00 40 10 05 F9 2F DD B0 44 01 40 58 50 04 01
>>> Searching rolling buffer (Current length: 184 bytes)
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x350F, Received CRC: 0x187C
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 25
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x93B9, Received CRC: 0x93B9
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 25 ---
[IoHomeNode::parseFrame] Raw bytes: F6 00 00 00 3F FA B7 10 00 01 43 00 00 00 00 0D C3 D3 7C 18 F5 FC AE B9 93
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 25
[IoHomeNode::validateFrameCrc] Calculated CRC: 0x93B9, Received CRC: 0x93B9
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF6 (Binary: 11110110), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 00003F, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 23 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0DC3, MAC (first 6 bytes): D3 7C 18 F5 FC AE
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0xD3, Expected 0xAA
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #2 (CRC OK) <<<
    Command: 0x00 | Source: 00003F | Dest: FAB710
    >>> AES MAC VERIFICATION FAILED (Or No Keys) <<<
>>> Searching rolling buffer (Current length: 67 bytes)
[RAW] Received packet! RSSI: -19.00 | Length: 255
      Data: 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 40 15 05 FF 00 56 15 1C 63 33 C6 57 04 D3 5D D9 52 7F FF C0 00 00 01 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 FF 66 43 F0 04 01 00 5F 92 FD DB 04 40 92 05 FF 40 58 50 05 41 7F C0 15 85 47 18 CC F1 95 C1 34 D7 76 54 9F FF E0 00 00 07 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FD 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 57 FD 99 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 16 0F
>>> Searching rolling buffer (Current length: 185 bytes)
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 25
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xF529, Received CRC: 0xFF05
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xF253, Received CRC: 0xF253
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 00 3F FA B7 10 20 02 FF 01 43 00 05 FF 00 0D C5 8C E6 4C 07 96 DD 53 F2
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xF253, Received CRC: 0xF253
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 00003F, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x20
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0DC5, MAC (first 6 bytes): 8C E6 4C 07 96 DD
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x8C, Expected 0x50
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #3 (CRC OK) <<<
    Command: 0x20 | Source: 00003F | Dest: FAB710
    >>> AES MAC VERIFICATION FAILED (Or No Keys) <<<
>>> Searching rolling buffer (Current length: 65 bytes)
[RAW] Received packet! RSSI: -19.50 | Length: 255
      Data: 0F C0 10 04 01 7E 4B F7 6C 11 02 48 17 FD 01 61 40 15 05 FF 00 56 15 1C 63 33 C6 57 04 D3 5D D9 52 7F FF C0 00 00 00 00 00 00 00 00 00 74 C6 23 C5 F6 B3 BC 55 D4 7D 6F 4C BF FF FF FF FF FF FF FF FF FF FF FF FF FF D5 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 55 7F D9 90 FC 01 00 50 17 EC BF 76 C1 10 05 01 61 40 10 04 03 09 C0 10 05 61 31 CD 91 14 77 02 5F 54 05 9D 1E FF FF C0 00 00 00 7F FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF EA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA
>>> Searching rolling buffer (Current length: 176 bytes)
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xC9FF, Received CRC: 0xE68C
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xBC73, Received CRC: 0xBC73
[IoHomeNode::parseFrame] --- Starting parse for frame of length: 27 ---
[IoHomeNode::parseFrame] Raw bytes: F8 00 00 01 BF FA B7 10 00 01 43 00 00 80 C8 00 00 0D C6 36 44 DC 20 5F 01 73 BC
[IoHomeNode::validateFrameCrc] Validating CRC for frame of length: 27
[IoHomeNode::validateFrameCrc] Calculated CRC: 0xBC73, Received CRC: 0xBC73
[IoHomeNode::parseFrame] Decoded Header: Ctrl0=0xF8 (Binary: 11111000), Ctrl1=0x00 (Binary: 0)
[IoHomeNode::parseFrame] Source MAC: 0001BF, Dest MAC: FAB710
[IoHomeNode::parseFrame] Command ID: 0x00
[IoHomeNode::parseFrame] Declared total body length from Ctrl0: 25 bytes
[IoHomeNode::parseFrame] Assuming security footer length: 8 (Counter: 2, MAC: 6)
[IoHomeNode::parseFrame] Extracted Security Footer: Counter=0x0DC6, MAC (first 6 bytes): 36 44 DC 20 5F 01
[IoHomeNode::parseFrame] ERROR: MAC mismatch at byte 0: Received 0x36, Expected 0x95
>>> SUCCESSFULLY RECEIVED IOHOME FRAME #4 (CRC OK) <<<
    Command: 0x00 | Source: 0001BF | Dest: FAB710
    >>> AES MAC VERIFICATION FAILED (Or No Keys) <<<
>>> Searching rolling buffer (Current length: 50 bytes)
Freq: 869.85 MHz | RSSI: -114.50 dBm | STATUS: OK
