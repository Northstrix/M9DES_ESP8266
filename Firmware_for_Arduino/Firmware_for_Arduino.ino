// M9DES implemented on ESP8266
// Firmware for the Arduino
// Distributed under the MIT License
// © Copyright Maxim Bortnikov 2021
// For more information please visit
// https://github.com/Northstrix/M9DES_ESP8266
#include <PS2Keyboard.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(5, 4); // RX, TX
const int DataPin = 8;
const int IRQpin =  3;
#include "GBUS.h"
PS2Keyboard keyboard;
GBUS bus(&mySerial, 5, 20);

struct myStruct {
  char x;
  byte sc;
};

void setup() {
  delay(1000);
  keyboard.begin(DataPin, IRQpin);
  Serial.begin(115200);
  mySerial.begin(9600);
  Serial.println("Keyboard Test:");
}

void loop() {
  if (keyboard.available()) {
    myStruct data;
    // read the next key
    char c = keyboard.read();
    data.sc = 0;
    // check for some of the special keys
    if (c == PS2_ENTER) {
      data.sc = 1;
      bus.sendData(3, data);
    } else if (c == PS2_UPARROW) {
      data.sc = 2;
      bus.sendData(3, data);
    } else if (c == PS2_DELETE) {
      data.sc = 3;
      bus.sendData(3, data);
    } else {
      
      // otherwise, just print all normal characters
      Serial.print(c);
      data.x = c;
      bus.sendData(3, data);
    }
  }
}
