/**
 * @file    pin_config.h
 * @brief   Hardware pin assignments and timing constants for rtos-hrt-firmware
 *
 * All physical pin numbers and board-level constants live here.
 * No other file should hardcode pin numbers — always reference these macros.
 * If the wiring changes, this is the only file that needs to be updated.
 *
 * Target board: Arduino Uno R3 (ATmega328P)
 *
 * Author : George Mahfood | Baremetal Labs
 */

#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// RGB LED (common cathode)
// PWM-capable pins required — D3, D5, D6 on the Uno are Timer-driven PWM
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_LED_R   6   // Timer0 — PWM red channel
#define PIN_LED_G   5   // Timer0 — PWM green channel
#define PIN_LED_B   3   // Timer2 — PWM blue channel

// ─────────────────────────────────────────────────────────────────────────────
// Push buttons (active LOW — INPUT_PULLUP, pressed = GND)
//
// D2 is used for the reaction button because it is the only pin on the Uno
// that supports INT0 (hardware interrupt 0). This gives us zero-latency
// response capture via ISR rather than polling — critical for accuracy.
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_BTN_REACT   2   // INT0 — hardware interrupt, reaction button
#define PIN_BTN_START   4   // Polled — start game
#define PIN_BTN_RESET   7   // Polled — return to idle from any state

// ─────────────────────────────────────────────────────────────────────────────
// Piezo buzzer (passive — driven with tone())
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_BUZZER  8

// ─────────────────────────────────────────────────────────────────────────────
// 7-segment display — MAX7219 SPI interface
// D11/D12/D13 are the Uno's hardware SPI bus, but MAX7219 works fine
// on any digital pins using software SPI via the LedControl library.
// We use D10-D12 to keep D13 free (it has an onboard LED that can interfere).
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_SEG_DIN 11  // MAX7219 data in
#define PIN_SEG_CLK 12  // MAX7219 clock
#define PIN_SEG_CS  10  // MAX7219 chip select (active LOW)

// ─────────────────────────────────────────────────────────────────────────────
// Potentiometer — analog difficulty control
// Maps 0–1023 ADC reading to DELAY_MIN_MS–DELAY_MAX_MS stimulus range
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_POT     A0

// ─────────────────────────────────────────────────────────────────────────────
// I2C — LCD 1602
// Fixed hardware pins on the ATmega328P — cannot be reassigned
// ─────────────────────────────────────────────────────────────────────────────
// SDA → A4  |  SCL → A5  (defined by hardware, not configurable)

// ─────────────────────────────────────────────────────────────────────────────
// Timing constants
//
// DELAY_MIN_MS / DELAY_MAX_MS — the random window for stimulus delay.
// The potentiometer scales between these two values, giving the player
// a difficulty dial from "easy" (long, predictable-ish wait) to
// "hard" (short, unpredictable wait).
//
// DEBOUNCE_MS — mechanical buttons bounce for 5–20ms. 50ms is conservative
// and reliable. Do not go below 20ms or you will get phantom presses.
//
// DISPLAY_HOLD_MS — how long the result stays on screen before returning
// to idle. 2 seconds is comfortable for reading a 3-4 digit number.
// ─────────────────────────────────────────────────────────────────────────────
#define DELAY_MIN_MS    500
#define DELAY_MAX_MS    3000
#define DEBOUNCE_MS     50
#define DISPLAY_HOLD_MS 2000