/**
 * @file    test_hardware.cpp
 * @brief   Hardware validation sketch — tests each peripheral in isolation
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 *
 * PURPOSE:
 *   Flash this INSTEAD of the main firmware to validate wiring before
 *   trusting the full RTOS system. Each peripheral is tested one at a
 *   time with serial prompts and visual/audible confirmation.
 *
 * USAGE:
 *   1. Comment out main.cpp (or temporarily rename it)
 *   2. Build and flash this file
 *   3. Open serial monitor at 115200 baud
 *   4. Follow the prompts — press ENTER to advance through each test
 *   5. When all tests pass, revert to main.cpp and flash the real firmware
 *
 * NOTE:
 *   This file has its own setup() and loop() — it cannot coexist with
 *   main.cpp in the same build. Only one should be active at a time.
 *   You can manage this by placing test files in a separate PlatformIO
 *   environment or by temporarily renaming main.cpp.
 */

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "pin_config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Onboard LED — PC13 on the Black Pill (active LOW)
// Used as a basic "is the board alive?" indicator
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_ONBOARD_LED PC13

// ─────────────────────────────────────────────────────────────────────────────
// Test helpers
// ─────────────────────────────────────────────────────────────────────────────

static uint8_t testNumber  = 0;
static uint8_t passCount   = 0;
static uint8_t failCount   = 0;
static const uint8_t TOTAL_TESTS = 11;

/**
 * @brief   Print a test header with number and description
 */
static void printTestHeader(const char* name) {
    testNumber++;
    Serial.println();
    Serial.println("────────────────────────────────────────");
    Serial.print("TEST ");
    Serial.print(testNumber);
    Serial.print("/");
    Serial.print(TOTAL_TESTS);
    Serial.print(": ");
    Serial.println(name);
    Serial.println("────────────────────────────────────────");
}

/**
 * @brief   Wait for the user to press ENTER in the serial monitor
 */
static void waitForEnter() {
    Serial.println(">> Press ENTER to continue...");
    while (!Serial.available()) { delay(10); }
    while (Serial.available()) { Serial.read(); }  // flush buffer
}

/**
 * @brief   Ask user to confirm pass/fail visually
 * @return  true if user typed 'y', false otherwise
 */
static bool askPassFail(const char* question) {
    Serial.print(">> ");
    Serial.print(question);
    Serial.println(" (y/n)");

    while (!Serial.available()) { delay(10); }
    char c = Serial.read();
    while (Serial.available()) { Serial.read(); }  // flush buffer

    if (c == 'y' || c == 'Y') {
        Serial.println("   PASS ✓");
        passCount++;
        return true;
    } else {
        Serial.println("   FAIL ✗ — check wiring");
        failCount++;
        return false;
    }
}

/**
 * @brief   Record an automatic pass (no user confirmation needed)
 */
static void autoPass(const char* msg) {
    Serial.print("   PASS ✓ — ");
    Serial.println(msg);
    passCount++;
}

/**
 * @brief   Record an automatic fail
 */
static void autoFail(const char* msg) {
    Serial.print("   FAIL ✗ — ");
    Serial.println(msg);
    failCount++;
}

// ─────────────────────────────────────────────────────────────────────────────
// Interrupt test support
// ─────────────────────────────────────────────────────────────────────────────
static volatile bool interruptFired = false;

void testISR() {
    interruptFired = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test functions
// ─────────────────────────────────────────────────────────────────────────────

static void testSerial() {
    printTestHeader("Serial communication");
    Serial.println("If you can read this, serial is working.");
    autoPass("serial output confirmed");
}

static void testOnboardLED() {
    printTestHeader("Onboard LED (PC13)");
    Serial.println("The onboard LED should blink 3 times...");

    pinMode(PIN_ONBOARD_LED, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_ONBOARD_LED, LOW);     // Active LOW
        delay(300);
        digitalWrite(PIN_ONBOARD_LED, HIGH);
        delay(300);
    }

    askPassFail("Did the onboard LED blink 3 times?");
}

static void testStartButton() {
    printTestHeader("Start button (PB1) — polled");
    Serial.println("Press and hold the START button...");

    pinMode(PIN_BTN_START, INPUT_PULLUP);

    uint32_t timeout = millis() + 10000;
    bool pressed = false;

    while (millis() < timeout) {
        if (digitalRead(PIN_BTN_START) == LOW) {
            pressed = true;
            break;
        }
        delay(10);
    }

    if (pressed) {
        autoPass("start button press detected");
        Serial.println("   Release the button now.");
        while (digitalRead(PIN_BTN_START) == LOW) { delay(10); }
        Serial.println("   Release confirmed.");
    } else {
        autoFail("no press detected within 10 seconds");
    }
}

static void testReactButtonPolled() {
    printTestHeader("React button (PB0) — polled read");
    Serial.println("Press and hold the REACT button...");

    pinMode(PIN_BTN_REACT, INPUT_PULLUP);

    uint32_t timeout = millis() + 10000;
    bool pressed = false;

    while (millis() < timeout) {
        if (digitalRead(PIN_BTN_REACT) == LOW) {
            pressed = true;
            break;
        }
        delay(10);
    }

    if (pressed) {
        autoPass("react button press detected via polling");
        Serial.println("   Release the button now.");
        while (digitalRead(PIN_BTN_REACT) == LOW) { delay(10); }
        Serial.println("   Release confirmed.");
    } else {
        autoFail("no press detected within 10 seconds");
    }
}

static void testReactButtonInterrupt() {
    printTestHeader("React button (PB0) — EXTI interrupt");
    Serial.println("Press the REACT button to test the interrupt...");

    interruptFired = false;
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_REACT), testISR, FALLING);

    uint32_t timeout = millis() + 10000;
    while (!interruptFired && millis() < timeout) {
        delay(10);
    }

    detachInterrupt(digitalPinToInterrupt(PIN_BTN_REACT));

    if (interruptFired) {
        autoPass("EXTI interrupt fired on falling edge");
    } else {
        autoFail("interrupt did not fire within 10 seconds");
    }
}

static void testRGBIndividual() {
    printTestHeader("RGB LED — individual channels");

    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);

    // All off first
    analogWrite(PIN_LED_R, 0);
    analogWrite(PIN_LED_G, 0);
    analogWrite(PIN_LED_B, 0);

    Serial.println("RED channel (PB4) should be on...");
    analogWrite(PIN_LED_R, 255);
    delay(1500);
    analogWrite(PIN_LED_R, 0);

    Serial.println("GREEN channel (PB5) should be on...");
    analogWrite(PIN_LED_G, 255);
    delay(1500);
    analogWrite(PIN_LED_G, 0);

    Serial.println("BLUE channel (PA8) should be on...");
    analogWrite(PIN_LED_B, 255);
    delay(1500);
    analogWrite(PIN_LED_B, 0);

    askPassFail("Did you see RED, then GREEN, then BLUE?");
}

static void testRGBPWM() {
    printTestHeader("RGB LED — PWM fade");
    Serial.println("All three channels will fade up and down...");

    for (int brightness = 0; brightness <= 255; brightness += 5) {
        analogWrite(PIN_LED_R, brightness);
        analogWrite(PIN_LED_G, brightness);
        analogWrite(PIN_LED_B, brightness);
        delay(15);
    }
    for (int brightness = 255; brightness >= 0; brightness -= 5) {
        analogWrite(PIN_LED_R, brightness);
        analogWrite(PIN_LED_G, brightness);
        analogWrite(PIN_LED_B, brightness);
        delay(15);
    }

    askPassFail("Did the LED smoothly fade up and down (white)?");
}

static void testBuzzer() {
    printTestHeader("Piezo buzzer (PB8)");
    Serial.println("Playing three tones...");

    tone(PIN_BUZZER, 440, 300);     // A4
    delay(400);
    tone(PIN_BUZZER, 880, 300);     // A5
    delay(400);
    tone(PIN_BUZZER, 1760, 300);    // A6
    delay(400);
    noTone(PIN_BUZZER);

    askPassFail("Did you hear three ascending tones?");
}

static void testPotentiometer() {
    printTestHeader("Potentiometer (PA0) — ADC read");
    Serial.println("Turn the potentiometer fully LEFT, then fully RIGHT.");
    Serial.println("Watching ADC values for 8 seconds...");
    Serial.println("Expected range: 0 to 4095 (12-bit ADC)");
    Serial.println();

    uint16_t minVal = 4095;
    uint16_t maxVal = 0;

    uint32_t endTime = millis() + 8000;
    while (millis() < endTime) {
        uint16_t val = analogRead(PIN_POT);
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;

        Serial.print("   ADC: ");
        Serial.print(val);
        Serial.print("  [min: ");
        Serial.print(minVal);
        Serial.print("  max: ");
        Serial.print(maxVal);
        Serial.println("]");

        delay(250);
    }

    Serial.println();
    Serial.print("   Range observed: ");
    Serial.print(minVal);
    Serial.print(" to ");
    Serial.println(maxVal);

    if (maxVal - minVal > 2000) {
        autoPass("potentiometer has good range");
    } else {
        Serial.println("   WARNING: narrow range detected — pot may not be wired correctly");
        askPassFail("Does the range look correct for your wiring?");
    }
}

static void testLCD() {
    printTestHeader("LCD 1602 I2C (PB6/PB7)");
    Serial.println("Attempting to initialize LCD at address 0x27...");

    LiquidCrystal_I2C lcd(0x27, 16, 2);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("rtos-hrt TEST");
    lcd.setCursor(0, 1);
    lcd.print("Baremetal Labs");

    askPassFail("Does the LCD show 'rtos-hrt TEST' on line 1?");
}

// ─────────────────────────────────────────────────────────────────────────────
// Summary
// ─────────────────────────────────────────────────────────────────────────────

static void printSummary() {
    Serial.println();
    Serial.println("════════════════════════════════════════");
    Serial.println("        HARDWARE TEST SUMMARY");
    Serial.println("════════════════════════════════════════");
    Serial.print("   PASSED: ");
    Serial.println(passCount);
    Serial.print("   FAILED: ");
    Serial.println(failCount);
    Serial.print("   TOTAL:  ");
    Serial.println(TOTAL_TESTS);
    Serial.println();

    if (failCount == 0) {
        Serial.println("   ALL TESTS PASSED — ready for firmware!");
    } else {
        Serial.println("   Fix failed tests before flashing firmware.");
    }

    Serial.println("════════════════════════════════════════");
}

// ─────────────────────────────────────────────────────────────────────────────
// Arduino entry points
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("════════════════════════════════════════");
    Serial.println("  rtos-hrt-firmware HARDWARE TEST SUITE");
    Serial.println("  STM32F411 Black Pill | Baremetal Labs");
    Serial.println("════════════════════════════════════════");
    Serial.println();
    Serial.println("This sketch tests each peripheral in isolation.");
    Serial.println("Follow the prompts and respond y/n when asked.");
    Serial.println();

    waitForEnter();

    // Run all tests in wiring order
    testSerial();               // Test 1
    testOnboardLED();           // Test 2
    testStartButton();          // Test 3
    testReactButtonPolled();    // Test 4
    testReactButtonInterrupt(); // Test 5
    testRGBIndividual();        // Test 6
    testRGBPWM();               // Test 7
    testBuzzer();               // Test 8
    testPotentiometer();        // Test 9
    testLCD();                  // Test 10 — wait, that's only 10

    // Test 11: combined rapid-fire I/O check
    printTestHeader("Combined I/O — rapid state cycle");
    Serial.println("Simulating a game cycle: LED green → buzzer → LCD update");

    analogWrite(PIN_LED_R, 0);
    analogWrite(PIN_LED_G, 255);
    analogWrite(PIN_LED_B, 0);
    delay(500);

    tone(PIN_BUZZER, 880, 200);
    delay(300);
    noTone(PIN_BUZZER);

    {
        LiquidCrystal_I2C lcd(0x27, 16, 2);
        lcd.init();
        lcd.backlight();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Time: 0234 ms");
        lcd.setCursor(0, 1);
        lcd.print("Nice reaction!");
    }

    delay(500);
    analogWrite(PIN_LED_G, 0);

    askPassFail("Did you see green LED, hear buzzer, and see LCD update?");

    // Print final summary
    printSummary();
}

void loop() {
    // Nothing — all tests run once in setup()
    delay(1000);
}