#include "IoHomeCrypto.h"
#include "IoHome.h"
#include <mbedtls/aes.h>
#include <algorithm>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

bool IoHomeCrypto::decryptTransferKey(const IoHomeFrame_t& parsedFrame, uint8_t* outKey) {
    uint8_t transfer_iv[16];
    // The IV is the Source MAC repeated to fill 16 bytes.
    // It must use the exact Over-The-Air byte order (n0, n1, n2).
    uint8_t mac_bytes[3] = { parsedFrame.sourceMac.n0, parsedFrame.sourceMac.n1, parsedFrame.sourceMac.n2 };
    for (int i = 0; i < 16; i++) {
        transfer_iv[i] = mac_bytes[i % 3];
    }

    mbedtls_aes_context aes_transfer;
    mbedtls_aes_init(&aes_transfer);
    // Global io-homecontrol Transfer Key
    const uint8_t transfer_key[16] = {0x34, 0xC3, 0x46, 0x6E, 0xD8, 0x8F, 0x4E, 0x8E, 0x16, 0xAA, 0x47, 0x39, 0x49, 0x88, 0x43, 0x73};
    mbedtls_aes_setkey_enc(&aes_transfer, transfer_key, 128);
    uint8_t encrypted_iv[16];
    mbedtls_aes_crypt_ecb(&aes_transfer, MBEDTLS_AES_ENCRYPT, transfer_iv, encrypted_iv);
    mbedtls_aes_free(&aes_transfer);

    // Extract the true Stack Key by XORing the encrypted payload with the AES-encrypted IV.
    for (int i = 0; i < 16; i++) {
        outKey[i] = parsedFrame.payload[i] ^ encrypted_iv[i];
    }

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.print("    [IoHomeCrypto] Plaintext Payload (Reference): ");
    for (int i = 0; i < 16; i++) Serial.printf("%02X ", parsedFrame.payload[i]);
    Serial.println();
#endif

    return true;
}

void IoHomeCrypto::generateMac(const IoHomeFrame_t& parsedFrame, const uint8_t* frameData, uint16_t sequenceCounter, const uint8_t* key, uint8_t* outMac) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);

    bool isOneWay = (parsedFrame.ctrlByte0 & 0x20) != 0;
    uint8_t iv[16];
    size_t cmd_payload_len = 1 + parsedFrame.payload.size();
    uint8_t cmd_payload_buf[32] = {0};
    cmd_payload_buf[0] = parsedFrame.commandId;
    for(size_t i = 0; i < parsedFrame.payload.size(); i++) {
        cmd_payload_buf[1 + i] = parsedFrame.payload[i];
    }

    for(size_t i = 0; i < 8; i++) {
        iv[i] = (i < cmd_payload_len) ? cmd_payload_buf[i] : 0x55;
    }

    // Checksum is ONLY calculated over Cmd + Payload.
    size_t checksum_len = cmd_payload_len;
    uint8_t c1 = 0, c2 = 0;
    for(size_t i = 0; i < checksum_len; i++) {
        uint8_t tmp = cmd_payload_buf[i] ^ c2;
        uint8_t next_c1 = (c1 << 1) & 0xFE;
        if ((c1 & 0x80) == 0) {
            if (tmp >= 128) next_c1 |= 1;
            c1 = next_c1;
            c2 = (tmp << 1) & 0xFF;
        } else {
            if (tmp >= 128) next_c1 |= 1;
            c1 = next_c1 ^ 0x55;
            c2 = ((tmp << 1) ^ 0x5B) & 0xFF;
        }
    }
    iv[8] = c1;
    iv[9] = c2;

    if (isOneWay) {
        iv[10] = (uint8_t)(sequenceCounter >> 8);
        iv[11] = (uint8_t)(sequenceCounter & 0xFF);
        iv[12] = 0x55; iv[13] = 0x55; iv[14] = 0x55; iv[15] = 0x55;
    } else {
        for (int i=10; i<16; i++) iv[i] = 0x00;
    }

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.print("[IoHomeCrypto] AES IV Block : ");
    for(size_t i = 0; i < 16; i++) Serial.printf("%02X ", iv[i]);
    Serial.println();
#endif

    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, iv, outMac);
    mbedtls_aes_free(&aes);
}

bool IoHomeCrypto::verifyMac(const IoHomeFrame_t& parsedFrame, const uint8_t* frameData, uint16_t rxCounter, const uint8_t* rxMac, size_t macLen, const uint8_t* activeKey) {
    uint8_t expected_block[16] = {0};
    generateMac(parsedFrame, frameData, rxCounter, activeKey, expected_block);

#if defined(ARDUINO) && defined(DEBUG_IOHOME)
    Serial.print("[IoHomeCrypto] Expected MAC : ");
    for(int i = 0; i < 16; i++) Serial.printf("%02X ", expected_block[i]);
    Serial.println();
    Serial.print("[IoHomeCrypto] Received MAC : ");
    for(size_t i = 0; i < macLen; i++) Serial.printf("%02X ", rxMac[i]);
    Serial.println();
#endif

    for (size_t i = 0; i < macLen; i++) {
        if (rxMac[i] != expected_block[i]) {
#if defined(ARDUINO) && defined(DEBUG_IOHOME)
            Serial.printf("[IoHomeCrypto] ERROR: MAC mismatch at byte %u: Received 0x%02X, Expected 0x%02X\n", i, rxMac[i], expected_block[i]);
#endif
            return false;
        }
    }
    return true;
}
