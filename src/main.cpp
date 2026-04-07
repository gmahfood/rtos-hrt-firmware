#include <Arduino.h>

void setup() {
    pinMode(PC13, OUTPUT);
}

void loop() {
    digitalWrite(PC13, LOW);  // LOW = on for Black Pill
    delay(500);
    digitalWrite(PC13, HIGH); // HIGH = off
    delay(500);
}