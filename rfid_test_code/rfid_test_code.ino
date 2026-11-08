#include <Wire.h>
#include <Adafruit_PN532.h>

// IRQ and RESET are just required for it to work, they are unused
#define PN532_IRQ   2
#define PN532_RESET 3

Adafruit_PN532 rfid(PN532_IRQ, PN532_RESET);

void setup() {
  Serial.begin(115200);
  rfid.begin();

//checking to see if rfid reader (PN532) is connected
//loops until it is
//for setup purposes
  uint32_t version = rfid.getFirmwareVersion();
  if (!version) {
    Serial.println("PN532 not found. Check wiring and DIP switches (I2C mode).");
    while (1);
  }
  Serial.println("PN532 ready. Scan a tag...");
}

void loop() {
  uint8_t uid[7];       // buffer for the UID (up to 7 bytes)
  uint8_t uidLength;

  // waits up to 500ms for a tag
  if (rfid.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 500)) {
    Serial.print("UID:");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i] < 0x10 ? " 0" : " ");
      Serial.print(uid[i], HEX);
    }
    Serial.println();
    delay(1000);   // avoid spamming the same tag
  }
}