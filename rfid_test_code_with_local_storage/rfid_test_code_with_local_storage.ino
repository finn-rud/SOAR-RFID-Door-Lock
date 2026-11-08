#include <Wire.h>
#include <Adafruit_PN532.h>
#include <EEPROM.h>

#define PN532_IRQ   2
#define PN532_RESET 3
Adafruit_PN532 rfid(PN532_IRQ, PN532_RESET);

// ---- config ----
const byte UID_LEN = 4;                                    // your tags are 4 bytes
const byte MASTER[UID_LEN] = {0x80, 0xF4, 0xE2, 0xA4};     // master/admin tag
const int  MAX_TAGS = 60;                                  // EEPROM can hold ~63

// ---- EEPROM layout ----
// address 0   : number of stored tags (1 byte)
// address 1.. : tags, UID_LEN bytes each
const int ADDR_COUNT = 0;
const int ADDR_TAGS  = 1;

bool enrollMode = false;

void setup() {
  Serial.begin(115200);
  EEPROM.update(0, 0); 
  rfid.begin();
  if (!rfid.getFirmwareVersion()) {
    Serial.println("PN532 not found.");
    while (1);
  }
  Serial.print("Ready. Stored tags: ");
  Serial.println(EEPROM.read(ADDR_COUNT));
  Serial.println("Scan a tag. Scan master to toggle enroll mode.");
}

void loop() {
  byte uid[7];
  byte uidLength;

  if (!rfid.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 500)) return;
  if (uidLength != UID_LEN) { Serial.println("Unexpected UID length, ignoring."); delay(1000); return; }

  printUID(uid);

  if (isMaster(uid)) {
    enrollMode = !enrollMode;
    Serial.println(enrollMode ? " -> ENROLL MODE ON (scan a tag to add)" : " -> enroll mode off");
    delay(1000);
    return;
  }

  if (enrollMode) {
    enrollTag(uid);
    enrollMode = false;                   // one enroll per master press
  } else {
    if (checkTag(uid)) {
      Serial.println(" -> VALID (would open door)");
    } else {
      Serial.println(" -> not recognized");
    }
  }
  delay(1000);
}

// true if scanned uid equals the master tag
bool isMaster(byte *uid) {
  for (byte i = 0; i < UID_LEN; i++) if (uid[i] != MASTER[i]) return false;
  return true;
}

// loop through stored tags, true if uid matches one
bool checkTag(byte *uid) {
  byte count = EEPROM.read(ADDR_COUNT);
  for (byte t = 0; t < count; t++) {
    int base = ADDR_TAGS + t * UID_LEN;
    bool match = true;
    for (byte i = 0; i < UID_LEN; i++) {
      if (EEPROM.read(base + i) != uid[i]) { match = false; break; }
    }
    if (match) return true;
  }
  return false;
}

// append a uid to EEPROM; skip if duplicate or full
void enrollTag(byte *uid) {
  if (checkTag(uid)) { Serial.println(" -> already enrolled"); return; }
  byte count = EEPROM.read(ADDR_COUNT);
  if (count >= MAX_TAGS) { Serial.println(" -> storage full"); return; }

  int base = ADDR_TAGS + count * UID_LEN;
  for (byte i = 0; i < UID_LEN; i++) EEPROM.update(base + i, uid[i]);
  EEPROM.update(ADDR_COUNT, count + 1);
  Serial.print(" -> ENROLLED (now storing ");
  Serial.print(count + 1);
  Serial.println(" tags)");
}

void printUID(byte *uid) {
  Serial.print("UID:");
  for (byte i = 0; i < UID_LEN; i++) {
    Serial.print(uid[i] < 0x10 ? " 0" : " ");
    Serial.print(uid[i], HEX);
  }
}