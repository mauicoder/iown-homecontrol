#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "IoHome.h"

// 1. Radio Setup for Heltec V3 (SX1262)
// This creates the hardware driver for the SX1262 chip on your Heltec V3.2
// Pins: NSS=8, DIO1=14, NRST=12, BUSY=13
SX1262 radio = new Module(8, 14, 12, 13);

// This creates a generic pointer so our IoHome library doesn't need
// to know exactly which chip (SX1262 or SX1276) we are using.
PhysicalLayer* phy = (PhysicalLayer*)&radio;

// 2. IoHome Setup
IoHomeChannel_t currentChannel = { 868, 95 };
IoHomeNode node(phy, &currentChannel);

// Keys (Replace with your real keys if you have them, otherwise use zeros for sniffing)
uint8_t stack_key[16] = {0};
uint8_t system_key[16] = {0};
NodeId myId = {0x01, 0x02, 0x03};
NodeId targetId = {0x00, 0x00, 0x00};

void setup() {
  Serial.begin(115200);

  // 1. Power on Heltec Vext
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);
  delay(1000);
  Serial.println(F("--- iHC Sniffer (Heltec V3.2) Starting ---"));

  // 2. Start SX1262 Chip
  int state = radio.begin();
  if(state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[RADIO] Hardware Init Failed: ")); Serial.println(state);
    while(true);
  }

  // 3. Configure Power Regulator for Heltec V3
  state = radio.setRegulatorLDO();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[RADIO] LDO Config Failed: ")); Serial.println(state);
  }

  // 4. Initialize IoHome Protocol Logic (Now with state check!)
  state = node.begin(&currentChannel, myId, targetId, stack_key, system_key);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[PROTO] Protocol Init Failed: ")); Serial.println(state);
    while(true);
  }

  // 5. Enter RX Mode
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("[RADIO] Start RX Failed: ")); Serial.println(state);
    while(true);
  }

  Serial.println(F("[SYSTEM] Online. Sniffing 868.95 MHz..."));
}

unsigned long lastHop = 0;
int currentFreqIdx = 0;
float channels[] = {868.25, 868.95, 869.85};

void loop() {
  IoHomeFrame_t rxFrame;

  // 1. Check if the radio has caught anything on the CURRENT frequency
  if (node.loop(rxFrame)) {
      // SUCCESS! We caught a packet.
      // Print the info you already had:
      Serial.print(F("\n[RECV] Caught on ")); Serial.print(channels[currentFreqIdx]); Serial.println(" MHz");
      Serial.print(F("  -> Cmd: 0x")); Serial.print(rxFrame.commandId, HEX);
      Serial.print(F(" From: "));
      Serial.printf("%02X%02X%02X\n", rxFrame.sourceMac.n0, rxFrame.sourceMac.n1, rxFrame.sourceMac.n2);

      if (!rxFrame.isValid) {
          Serial.println(F("  -> Security check failed (This is normal if you don't have the Stack Key)"));
      } else {
          Serial.println(F("  -> Security Verified! Message is authentic."));
      }

      // Optional: don't hop immediately after a success, stay here for a second
      lastHop = millis() + 1000;
  }

  // 2. Frequency Hopping Logic
  // If we haven't seen anything for 400ms, switch to the next standard iHC channel
  if (millis() - lastHop > 400) {
    currentFreqIdx = (currentFreqIdx + 1) % 3;

    // Tell the radio to move
    phy->setFrequency(channels[currentFreqIdx]);

    // IMPORTANT: On SX1262 (Heltec V3), we must re-enter RX mode after a frequency change
    phy->startReceive();

    lastHop = millis();

    // Helpful for debugging setup: uncomment this if you aren't sure it's hopping
    // Serial.printf("Scanning %.2f...\n", channels[currentFreqIdx]);
  }

  yield(); // Keep the ESP32 background tasks (WiFi/Watchdog) happy
}
