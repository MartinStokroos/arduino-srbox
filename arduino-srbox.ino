/*
  * * * The Arduino Serial Response Box * * *

  Emulates the PST SRB 200A

  Better disable the 'auto-reset by serial connection function' from the Arduino.
  This guarantees a quick start after port connection.

  By: M. Stokroos
  Date: 29-10-2025

  * * * * * * * * * * * * * * * * * * * * * *
*/

#include <digitalWriteFast.h>

#define BASE_GPIO_OUT 2  // for UNO D2 (pin D0 and D1 are RX+TX)
#define BASE_GPIO_IN 14  // = A0

// pin defs
int LED_pin = 13; // Buttons enabled monitor

const bool outputInvert = false; // using transistor inverters to drive lamps?
const bool ncSwitch = false; // using normally closed contacts?
bool scanButtons = false;
unsigned char byteIn;
unsigned char inButtons, lastButtons = 0;

// Function prototypes
void writePort(uint8_t);



void setup() {

  Serial.begin(19200);

  // digital output pins:
  pinMode(LED_pin, OUTPUT);
  pinMode(11, OUTPUT);

  for (int N = 0; N < 8; N++) {
    pinMode(BASE_GPIO_OUT + N, OUTPUT);
  }

  // Use A0-A4 as digital input pins:
  for (int N = 0; N < 5; N++) {
    pinMode(BASE_GPIO_IN + N, INPUT_PULLUP);  // Use INPUT or INPUT_PULLUP to enable the internal pull-up resistors.
  }
  writePort(0); // reset port pins
}



void loop() {

  if (Serial.available() > 0) {

    byteIn = Serial.read();

    switch (byteIn) {
      case 0xA0:  // enable button scan
        scanButtons = true;
        break;

      case 0x20:  // disable button scan
        scanButtons = false;
        break;

      case 0x60 ... 0x7F:
        writePort(byteIn - 0x60);
        break;

      default:
        break;
    }
  }

  if (scanButtons) {

    digitalWriteFast(LED_pin, true);

    if (ncSwitch) {
      inButtons = PINC & 0x1F;  // Read 5 buttons at once
    }
    else {
      inButtons = ~PINC & 0x1F; // Read 5 buttons at once and invert.
    }
    Serial.write(inButtons);
  }
  else {
    digitalWriteFast(LED_pin, false)
  }

}




void writePort(uint8_t output) {
  int i;

  if (outputInvert) {
    // write out 8 bits, invert and split out over port PD and PB
    for (i = 0; i < 8; i++) {
      digitalWriteFast(BASE_GPIO_OUT + i, !(output & (1 << i)) );
    }
  }
  else {
    // write out 8 bits split out over port PD and PB
    for (i = 0; i < 8; i++) {
      digitalWriteFast(BASE_GPIO_OUT + i, output & (1 << i));
    }
  }
}
