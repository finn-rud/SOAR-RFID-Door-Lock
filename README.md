RFID Door Lock

A keyless entry system for the SOAR workshop at the University of Pittsburgh. Tap an RFID tag at the door and a stepper motor on the inside turns the latch, so anyone on the team can get in without tracking down the one person who has the key.

Status: In progress. The circuit is working end to end on a breadboard. Next step is moving it onto a PCB and mounting it on the workshop door.

Built by Finn Ruderman and Sam Agerbak.

The problem:

The workshop runs on a single physical key, and a lot of members need to get in. In practice that means texting around to find whoever's holding the key before anyone can work. This replaces that with RFID tags: encode a tag for each member and they can let themselves in.

How it works:
An RFID reader (PN532, in I2C mode) sits by the door.
When a tag is tapped, an Arduino Nano Every reads its UID and checks it against a list of approved tags in the firmware.
If the tag is approved, the Arduino pulses an A4988 driver, which runs a stepper motor mounted on the inside of the door.
The motor turns the latch and the door opens. An unknown tag does nothing.

Keeping the motor on the inside means the only thing on the outside is the reader, and the existing door hardware doesn't have to change.

Hardware:
Part	                      Role
PN532 RFID/NFC reader	     Reads tags (I2C mode)
Arduino Nano Every	        Controller
A4988 stepper driver	      Drives the motor
NEMA 17 stepper motor	     Turns the latch
RFID tags	                 One per member
Breadboard + jumper wires	 Prototype wiring
Motor power supply (~12 V)	Powers the A4988 / motor
Wiring

Exact Arduino pin assignments are set in the #define block at the top of the sketch (STEP, DIR, and EN for the A4988). At a glance:

PN532 → Arduino (I2C): SDA, SCL, VCC, GND. Set the PN532's DIP switches to I2C mode.
A4988 → Arduino: STEP, DIR, and EN to the pins defined in the sketch.
A4988 power: VMOT + GND to the motor supply (put a capacitor across VMOT/GND), VDD + GND to logic. Tie RESET to SLEEP.
A4988 → motor: the two coil pairs (1A/1B, 2A/2B) to the stepper's coils.

Double-check the A4988 current limit (the small trim pot) against your motor before running it for any length of time.

Getting it running:
Install the Adafruit PN532 library (and its dependency, Adafruit BusIO) from the Library Manager. The stepper is driven directly through the A4988's STEP/DIR pins, so no motor library is required.
Open the sketch, pick the right port under Tools → Port, and upload.
Open the Serial Monitor. On startup it reports whether the reader was found and then waits for a tag.
Adding an authorized tag
Upload the sketch and open the Serial Monitor.
Tap the new tag. Its UID prints to the monitor.
Add that UID to the approved-tags list in the sketch.
Re-upload. The tag now opens the door.

Roadmap:
 Working prototype on a breadboard
 Design and order a PCB to replace the breadboard
 Mount the stepper and latch mechanism on the workshop door
 Encode tags for the members who need access
