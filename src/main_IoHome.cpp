#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "IoHome.h" // Include the IoHome library header

// --- HELTEC V3.2 PIN MAPPING ---
#define HW_VEXT            36   // Power Rail (Active LOW)
#define HW_LED             35   // System LED
#define LORA_NSS           8    // Chip Select
#define LORA_DIO1          14   // Interrupt
#define LORA_NRST          12   // Reset
#define LORA_BUSY          13   // Busy Pin
#define LORA_SCK           9
#define LORA_MISO          11
#define LORA_MOSI          10

// --- RADIO OBJECT ---
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_NRST, LORA_BUSY);

// --- INTERRUPT HANDLING ---
// Flag to indicate that a packet was received.
// Needs to be volatile to be safely accessed from an ISR.
volatile bool receivedFlag = false;

// This function is called when a complete packet is received by the module.
// It must be marked with ICACHE_RAM_ATTR for ESP32.
#if defined(ESP32)
IRAM_ATTR
#endif
void setFlag(void) {
  receivedFlag = true;
}
// --- GLOBAL STATE ---
float targetFreq = 868.95;
uint32_t lastRssiUpdate = 0;

// --- IOHOME LIBRARY OBJECTS ---
// Define the channel based on targetFreq (868.95 MHz -> c0=868, c1=95)
IoHomeChannel_t ioHomeChannel = { .c0 = 868, .c1 = 95 };
// Placeholder NodeIDs and keys - replace with your actual values
NodeId sourceNodeId = {0x00, 0x00, 0x00};
NodeId destNodeId = {0x00, 0x00, 0x00};
uint8_t stackKey[16] = {0};
uint8_t systemKey[16] = {0};
IoHomeNode ioNode(&radio, &ioHomeChannel);

void setup() {
  // 1. MANDATORY POWER SEQUENCE (The "Yellow Arrow" Fix)
  pinMode(HW_VEXT, OUTPUT);
  digitalWrite(HW_VEXT, LOW); // Pull LOW to power the SX1262

  pinMode(HW_LED, OUTPUT);
  digitalWrite(HW_LED, LOW);

  // Initialize Serial and wait for connection
  Serial.begin(115200);
  while(!Serial);
  delay(1000);

  Serial.println(F("   HELTEC V3.2 IoHome NODE     "));
  Serial.println(F("==============================="));

  // 2. MANUAL RESET (Ensures Silicon is fresh)
  pinMode(LORA_NRST, OUTPUT);
  digitalWrite(LORA_NRST, LOW);
  delay(50);
  digitalWrite(LORA_NRST, HIGH);
  delay(200);

  // 3. SPI INITIALIZATION
  // NSS pin is managed by RadioLib, so it should not be passed to SPI.begin()
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI);

  // 4. RADIOLIB STARTUP
  Serial.print(F("Initializing Radio... "));
  // FSK Params: Freq, Bitrate (38.4), Dev (19.2), RX BW (156.2)

  int state = radio.beginFSK(targetFreq, 38.4, 19.2, 156.2);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("SUCCESS"));

    // --- CRITICAL V3.2 TUNING ---
    radio.setDio2AsRfSwitch(true); // LINK PHYSICAL ANTENNA
    radio.setTCXO(1.6);            // V3.2 uses 32MHz TCXO
    radio.setRegulatorLDO();       // Quieter noise floor for RSSI
    radio.calibrateImage(targetFreq);

    // --- CONFIGURE IOHOME PACKET PARAMETERS ---
    Serial.print(F("Setting packet parameters... "));
    state = radio.setPreambleLength(IOHOME_PREAMBLE_LEN);
    if (state == RADIOLIB_ERR_NONE) {
      state = radio.setCRC(0); // Hardware CRC Off
    }
    if (state == RADIOLIB_ERR_NONE) {
      // Must use Fixed length because the length byte is PN9 whitened!
      // 64 bytes is enough to safely capture the entire io-homecontrol packet + some trailing noise
      state = radio.fixedPacketLengthMode(64);
    }
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("SUCCESS"));
    } else {
      Serial.printf("FAILED, code %d\n", state);
    }

    // Set the actual sync word bytes
    Serial.print(F("Setting sync word... "));
    // io-homecontrol original hardware uses Direct Mode (bypassing the hardware Sync Word entirely).
    // The true physical Sync Word is the UART-encoded bits of "FF 33", which precisely evaluate to 0x57FD99!
    uint8_t syncWord[] = { 0x57, 0xFD, 0x99 };
    state = radio.setSyncWord(syncWord, 3);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("SUCCESS (0x57FD99)"));
    } else {
      Serial.printf("FAILED, code %d\n", state); // This is a critical failure
    }

    // Set the function to be called upon successful reception (interrupt)
    radio.setDio1Action(setFlag);

    // 5. START RECEIVING
    int startReceiveState = radio.startReceive();
    if (startReceiveState != RADIOLIB_ERR_NONE) {
      Serial.printf("Failed to start receive, code %d\n", startReceiveState);
      while (true); // Halt execution on critical error in setup
    }
    Serial.println(F("Radio is LISTENING."));

    // 6. INITIALIZE IOHOME LIBRARY
    Serial.print(F("Initializing IoHomeNode... "));
    state = ioNode.begin(&ioHomeChannel, sourceNodeId, destNodeId, stackKey, systemKey);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("SUCCESS"));
    } else {
        Serial.printf("FAILED (Error: %d)\n", state);
    }
  } else {
    Serial.printf("FAILED (Error: %d)\n", state);
    while(1);
  }
}

void loop() {
  // --- 1. MONITOR RSSI (Ambient Noise) ---
  if (millis() - lastRssiUpdate > 1000) {
    lastRssiUpdate = millis();

    // Get instantaneous RSSI to verify the antenna is "live"
    float currentRssi = radio.getRSSI(false); // Use 'false' for instantaneous RSSI

    Serial.print(F("RSSI: "));
    Serial.print(currentRssi);
    Serial.print(F(" dBm | "));

    if (currentRssi > -1.0) {
      Serial.println(F("STATUS: BLINDED (Hardware/Antenna Issue)"));
      digitalWrite(HW_LED, HIGH); // Constant LED means hardware fail
    } else {
      Serial.println(F("STATUS: OK"));
      // Short pulse to show life
      digitalWrite(HW_LED, HIGH); delay(10); digitalWrite(HW_LED, LOW);
    }
  }

  // --- 2. IOHOME PACKET HANDLING ---
  // Check if the interrupt flag has been set by the ISR
  if (receivedFlag) {
    // Reset the flag
    receivedFlag = false;

    // A packet was received, read it
    // Allocate 256 bytes to safely handle the maximum possible SX1262 FIFO size
    int len = radio.getPacketLength();
    byte byteArr[256] = {0};
    int state = radio.readData(byteArr, len);

    if (state == RADIOLIB_ERR_NONE) {
      // Packet was read successfully
      Serial.print(F("[RAW] Received packet! RSSI: "));
      Serial.print(radio.getRSSI());
      Serial.print(F(" | Length: "));
      Serial.println(len);

      // Print the raw data as hex
      Serial.print(F("      Data: "));
      for(int i = 0; i < len; i++) {
        if(byteArr[i] < 0x10) {
          Serial.print(F("0"));
        }
        Serial.print(byteArr[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // Packet was received, but is malformed
      Serial.println(F("[RAW] CRC error!"));

    } else {
      // Some other error occurred
      Serial.print(F("[RAW] Receive failed, code "));
      Serial.println(state);
    }

    // Put the radio back into listening mode
    radio.startReceive();
  }
}
