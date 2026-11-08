#include <AccelStepper.h>

//I2C Stuff
#include <Wire.h>
#include <Adafruit_PN532.h>

//arduino storage stuff
#include <EEPROM.h>

//necessary but unused pin declarations
#define PN532_IRQ 2
#define PN532_RESET 3

//initializing scanner object

Adafruit_PN532 rfid(PN532_IRQ, PN532_RESET);


#define handleOpen 500
#define EN_PIN 17
#define STEP_PIN 16
#define DIR_PIN 15

//setting up the three door states
enum DoorState {OPENING, CLOSING, CLOSED};
DoorState state = CLOSED;

//setting up pins from arduino to a4988
AccelStepper doorMotor(AccelStepper::DRIVER,STEP_PIN,DIR_PIN);

//arduino storage stuff
// ---- config ----
const byte UID_LEN = 4;                                    // your tags are 4 bytes
const byte MASTER[UID_LEN] = {0x80, 0xF4, 0xE2, 0xA4};     // master/admin tag
const int  MAX_TAGS = 60;                                  // EEPROM can hold ~63

// ---- EEPROM layout ----
// address 0   : number of stored tags (1 byte)
// address 1.. : tags, UID_LEN bytes each
const int ADDR_COUNT = 0;
const int ADDR_TAGS  = 1;
const int ADDR_MAGIC = 255;   
const byte MAGIC = 0x42;      

bool enrollMode = false;

void setup() {
  
  //tag scanning
  Serial.begin(115200);

  //erases storage if its a fresh boot, does nothing if not
  //makes sure the data is still stored
  if (EEPROM.read(ADDR_MAGIC) != MAGIC) {   
    EEPROM.update(ADDR_COUNT, 0);
    EEPROM.update(ADDR_MAGIC, MAGIC);
  } 

  EEPROM.update(ADDR_COUNT, 0);

  rfid.begin();

  if (!rfid.getFirmwareVersion()) {
    Serial.println("PN532 not found.");
    while (1);
  }

  Serial.print("Ready. Stored tags: ");
  Serial.println(EEPROM.read(ADDR_COUNT));
  Serial.println("Scan a tag. Scan master to toggle enroll mode.");

  //motor stuff
  doorMotor.setMaxSpeed(800);
  doorMotor.setAcceleration(400);
//inverting the enable pin because it's active low
  doorMotor.setPinsInverted(false, false, true);
  doorMotor.setEnablePin(EN_PIN);
//initially disabling the driver outputs so the motor doesn't stay on constantly
  doorMotor.disableOutputs();
}

void loop() {
  // --- motor state machine runs every loop ---
  switch (state) {
    case OPENING:
      if (doorMotor.distanceToGo() == 0) {
        delay(6000);
        doorMotor.moveTo(0);
        state = CLOSING;
      }
      break;
    case CLOSING:
      if (doorMotor.distanceToGo() == 0) {
        doorMotor.disableOutputs();
        state = CLOSED;
      }
      break;
  }
  doorMotor.run();

  // --- tag handling, only fires when a tag is present ---
  byte uid[7];
  byte uidLength;

  if (state != CLOSED) return; 

  if (rfid.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 20)) {
    if (uidLength != UID_LEN) {
      Serial.println("Unexpected UID length, ignoring.");
      delay(2000);
      return;
    }

    printUID(uid);

    if (isMaster(uid)) {
      enrollMode = !enrollMode;
      Serial.println(enrollMode ? " -> ENROLL MODE ON (scan a tag to add)" : " -> enroll mode off");
      delay(2000);
    } else if (enrollMode) {
      enrollTag(uid);
      enrollMode = false;
      delay(2000);
    } else if (checkTag(uid)) {
      Serial.println(" -> VALID DOOR OPENING");
      OpenDoor();
    } else {
      Serial.println(" -> not recognized");
      delay(2000);
    }
  }
}

//////////////////all functions//////////////////////
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

void OpenDoor() {
  if (state == CLOSED) {
    doorMotor.enableOutputs();
    doorMotor.moveTo(handleOpen);
    state = OPENING;
  }
}