/**
 * @file    pin_config.h
 * @brief   Hardware pin assignments and timing constants for rtos-hrt-firmware
 *
 * All physical pin numbers and board-level constants live here.
 * No other file should hardcode pin numbers — always reference these macros.
 * If the wiring changes, this is the only file that needs to be updated.
 *
 * Target board: STM32F411 Black Pill (WeAct Studio v3.1)
 * Core:         ARM Cortex-M4 @ 100MHz
 * Framework:    Arduino (STM32duino) + FreeRTOS
 *
 * Board pin layout reference (ST-Link header facing up):
 *
 *   LEFT SIDE (top to bottom)        RIGHT SIDE (bottom to top)
 *   ─────────────────────────        ─────────────────────────
 *   5V                               3V3
 *   G                                G
 *   3V3                              5V
 *   B10                              B9
 *   B2                               B8   ← Buzzer
 *   B1   ← Start button              B7   ← I2C1 SDA (LCD)
 *   B0   ← React button (EXTI)       B6   ← I2C1 SCL (LCD)
 *   A7   ← SPI1 MOSI (TFT Ph2)       B5   ← RGB green (TIM3 CH2)
 *   A6   ← SPI1 MISO (TFT Ph2)       B4   ← RGB red (TIM3 CH1)
 *   A5   ← SPI1 SCK  (TFT Ph2)       B3
 *   A4   ← SPI1 CS   (TFT Ph2)       A15
 *   A3                               A12
 *   A2                               A11
 *   A1                               A10
 *   A0   ← Potentiometer (ADC)       A9
 *   R    (NRST — do not use)         A8   ← RGB blue (TIM1 CH1)
 *   C15                              B15
 *   C14                              B14
 *   C13  (onboard LED)               B13
 *   VB   (VBAT)                      B12
 *
 * Author : George Mahfood | Baremetal Labs
 */

#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// RGB LED (common cathode)
//
// All three pins must support hardware PWM for smooth color mixing.
// STM32 timers drive PWM — each pin is tied to a specific timer channel.
//
// PB4 → TIM3 Channel 1 (red)
// PB5 → TIM3 Channel 2 (green)
// PA8 → TIM1 Channel 1 (blue)
//
// PA8 is on a different timer than red/green. This is fine — analogWrite()
// handles each channel independently. We use PA8 because PB6 (the next
// TIM4 PWM candidate) is already taken by I2C1 SCL.
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_LED_R   PB4     // Board label: B4  — right side, 8th from bottom
#define PIN_LED_G   PB5     // Board label: B5  — right side, 7th from bottom
#define PIN_LED_B   PA8     // Board label: A8  — right side, 13th from bottom

// ─────────────────────────────────────────────────────────────────────────────
// Push buttons (active LOW — INPUT_PULLUP, pressed = GND)
//
// React button uses EXTI (External Interrupt) for zero-latency capture.
// On the STM32F411, every GPIO can trigger EXTI, but PB0 maps to EXTI line 0
// which has its own dedicated IRQ handler (EXTI0_IRQn) — no shared vectors.
// This gives us a clean ISR → semaphore → task path for reaction timing.
//
// Start button is polled with software debounce. Latency doesn't matter
// here — we're just transitioning from IDLE to WAITING.
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_BTN_REACT   PB0 // Board label: B0  — left side, 7th from top
#define PIN_BTN_START   PB1 // Board label: B1  — left side, 6th from top

// ─────────────────────────────────────────────────────────────────────────────
// Piezo buzzer (passive — driven with tone())
//
// PB8 is a general-purpose digital output with no peripheral conflicts.
// tone() generates a software-driven square wave at the requested frequency.
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_BUZZER  PB8     // Board label: B8  — right side, 4th from bottom

// ─────────────────────────────────────────────────────────────────────────────
// Potentiometer — analog difficulty control
//
// PA0 is ADC1 channel 0 on the STM32F411. analogRead() returns 0–4095
// (12-bit ADC) compared to the Uno's 0–1023 (10-bit). The stimulus task
// maps this range to DELAY_MIN_MS–DELAY_MAX_MS.
//
// NOTE: STM32 ADC is 12-bit (0–4095), not 10-bit like the ATmega328P.
//       Update any map() calls to use 4095 as the input maximum.
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_POT     PA0     // Board label: A0  — left side, 15th from top

// ─────────────────────────────────────────────────────────────────────────────
// I2C1 — LCD 1602 (Phase 1 display)
//
// PB7 and PB6 are the default I2C1 SDA/SCL pins on the STM32F411.
// The Wire library uses these automatically — no remapping needed.
// These are documented here for wiring reference, not used as macros.
//
// PB7 → I2C1 SDA   Board label: B7  — right side, 5th from bottom
// PB6 → I2C1 SCL   Board label: B6  — right side, 6th from bottom
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// SPI1 — ILI9488 3.5" TFT display (Phase 2 upgrade, not wired yet)
//
// PA5/PA6/PA7 are the default SPI1 pins on the STM32F411.
// PA4 is used as chip select (directly adjacent on the board).
// These are documented here for future wiring — not used in Phase 1 code.
//
// PA4 → SPI1 CS    Board label: A4  — left side, 11th from top
// PA5 → SPI1 SCK   Board label: A5  — left side, 10th from top
// PA6 → SPI1 MISO  Board label: A6  — left side, 9th from top
// PA7 → SPI1 MOSI  Board label: A7  — left side, 8th from top
// ─────────────────────────────────────────────────────────────────────────────

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
//
// ADC_MAX — STM32F411 has a 12-bit ADC (0–4095). Used in map() calls
// for potentiometer scaling. The Uno used 1023 (10-bit).
// ─────────────────────────────────────────────────────────────────────────────
#define DELAY_MIN_MS    500
#define DELAY_MAX_MS    3000
#define DEBOUNCE_MS     20
#define DISPLAY_HOLD_MS 2000
#define ADC_MAX         4095