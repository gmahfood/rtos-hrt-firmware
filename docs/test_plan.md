# Test Plan — rtos-hrt-firmware

> George Mahfood | Baremetal Labs
> STM32F411 Black Pill | FreeRTOS

---

## Overview

This document defines the validation strategy for the rtos-hrt-firmware project. Testing is split into two phases: hardware bring-up (peripheral isolation) and integration validation (full firmware behavior).

---

## Phase 1: Hardware bring-up

**Tool:** `test/test_hardware.cpp` — standalone sketch flashed before main firmware.

**Purpose:** Validate each peripheral in isolation to confirm wiring before trusting the RTOS system. If a peripheral fails here, the problem is wiring or hardware — not firmware logic.

| # | Test | Pin(s) | Method | Pass criteria |
| - | ---- | ------ | ------ | ------------- |
| 1 | Serial output | USB | Automatic | Text visible in serial monitor at 115200 baud |
| 2 | Onboard LED | PC13 | Visual | LED blinks 3 times |
| 3 | Start button (polled) | PB1 | Interactive | digitalRead returns LOW when pressed, HIGH when released |
| 4 | React button (polled) | PB0 | Interactive | digitalRead returns LOW when pressed, HIGH when released |
| 5 | React button (EXTI) | PB0 | Interactive | ISR fires on falling edge, sets flag within 10 seconds |
| 6 | RGB LED channels | PB4, PB5, PA8 | Visual | Red, green, blue illuminate individually in sequence |
| 7 | RGB LED PWM | PB4, PB5, PA8 | Visual | Smooth fade up and down (white) — no flickering or stepping |
| 8 | Piezo buzzer | PB8 | Audible | Three ascending tones clearly audible |
| 9 | Potentiometer ADC | PA0 | Automatic + visual | ADC range exceeds 2000 counts when pot swept full range (0-4095 expected) |
| 10 | LCD 1602 I2C | PB6, PB7 | Visual | Text displayed on both rows at I2C address 0x27 |
| 11 | Combined I/O | All | Visual + audible | Green LED, buzzer tone, and LCD update occur in rapid sequence |

**Procedure:**

1. Wire one peripheral at a time
2. Flash `test_hardware.cpp`
3. Open serial monitor at 115200 baud
4. Follow prompts, respond y/n for each test
5. Record results below
6. Fix any failures before proceeding to the next peripheral
7. When all 11 tests pass, flash the main firmware

---

## Phase 2: Integration validation

**Tool:** Main firmware (`main.cpp`) with serial monitor at 115200 baud.

**Purpose:** Validate the complete RTOS system — task communication, state machine transitions, timing accuracy, and edge cases.

### Test 2.1 — Boot sequence

| Step | Action | Expected serial output | Expected hardware |
| ---- | ------ | ---------------------- | ----------------- |
| 1 | Power on / reset | `[rtos-hrt] booting...` | None |
| 2 | Scheduler starts | `[rtos-hrt] starting scheduler...` | LCD shows "rtos-hrt" splash for 2 seconds |
| 3 | Idle state | No further output | LCD shows "Press START to begin..." |

### Test 2.2 — Normal game cycle

| Step | Action | Expected serial output | Expected hardware |
| ---- | ------ | ---------------------- | ----------------- |
| 1 | Press START button | None | LCD shows "Get ready..." |
| 2 | Wait for stimulus | None | Random delay (0.5-3s) |
| 3 | LED fires | `[game] stimulus fired at XXXX ms` | Green LED on, LCD shows "GO! GO! GO!" |
| 4 | Press REACT button | `[game] reaction time: XXX ms` | Buzzer single tone, LED off |
| 5 | Result displayed | None | LCD shows "Time: XXX ms" for 2 seconds |
| 6 | Auto-return to idle | None | LCD shows "Press START to begin..." |

### Test 2.3 — High score

| Step | Action | Expected serial output | Expected hardware |
| ---- | ------ | ---------------------- | ----------------- |
| 1 | Complete a round | `[game] reaction time: XXX ms` | Normal result |
| 2 | Beat previous time | `[game] NEW HIGH SCORE: XXX` | Three ascending buzzer tones, LCD shows "** NEW RECORD **" |
| 3 | Auto-return | None | LCD returns to idle after 2 seconds |

### Test 2.4 — False start

| Step | Action | Expected serial output | Expected hardware |
| ---- | ------ | ---------------------- | ----------------- |
| 1 | Press START | None | LCD shows "Get ready..." |
| 2 | Press REACT before green LED | `[game] false start!` | Red LED on, low warning buzzer tone |
| 3 | Penalty pause | None | Red LED for ~1 second |
| 4 | Auto-return to idle | None | LED off, LCD shows "Press START to begin..." |

### Test 2.5 — Timeout (no reaction)

| Step | Action | Expected serial output | Expected hardware |
| ---- | ------ | ---------------------- | ----------------- |
| 1 | Press START | None | Normal countdown |
| 2 | Green LED fires | `[game] stimulus fired at XXXX ms` | Green LED on |
| 3 | Do NOT press react | `[game] timeout — no reaction` (after ~6s) | LED off, LCD returns to idle |

### Test 2.6 — Potentiometer difficulty scaling

| Step | Action | Expected | 
| ---- | ------ | -------- |
| 1 | Turn pot fully counterclockwise | Random delay clusters near 500ms (minimum) |
| 2 | Turn pot fully clockwise | Random delay spreads up to 3000ms (maximum) |
| 3 | Verify by running 5+ rounds at each extreme | Delay range should noticeably shift |

### Test 2.7 — Rapid repeated play

| Step | Action | Expected |
| ---- | ------ | -------- |
| 1 | Play 10 consecutive rounds without pausing | No crashes, no stale queue events, consistent behavior |
| 2 | Mash react button during result screen | No false starts on next round (queue flush working) |
| 3 | Press start rapidly during idle | Only one round starts (no double-trigger) |

### Test 2.8 — Timing accuracy

| Step | Action | Expected |
| ---- | ------ | -------- |
| 1 | Use phone stopwatch alongside the game | Reported reaction times should be within ~20ms of phone measurement |
| 2 | Run 5 rounds, record each time | Times should be consistent for similar reaction speeds (150-400ms typical) |
| 3 | Verify reaction time is never negative or zero | All values should be positive integers |

---

## Test results log

Record your results here as you test. Date each session.

```
Date: ____/____/____

Hardware bring-up:
  [ ] Test 1  — Serial
  [ ] Test 2  — Onboard LED
  [ ] Test 3  — Start button
  [ ] Test 4  — React button (polled)
  [ ] Test 5  — React button (EXTI)
  [ ] Test 6  — RGB LED channels
  [ ] Test 7  — RGB LED PWM
  [ ] Test 8  — Buzzer
  [ ] Test 9  — Potentiometer
  [ ] Test 10 — LCD 1602
  [ ] Test 11 — Combined I/O

Integration:
  [ ] Test 2.1 — Boot sequence
  [ ] Test 2.2 — Normal game cycle
  [ ] Test 2.3 — High score
  [ ] Test 2.4 — False start
  [ ] Test 2.5 — Timeout
  [ ] Test 2.6 — Potentiometer scaling
  [ ] Test 2.7 — Rapid repeated play
  [ ] Test 2.8 — Timing accuracy

Notes:
_____________________________________________
_____________________________________________
_____________________________________________
```

---

## Known limitations

- Reaction timing uses `millis()` (1ms resolution). Microsecond precision would require a dedicated hardware timer (Phase 2 consideration).
- High score resets on power cycle (RAM only). Flash persistence planned for Phase 2.
- LCD 1602 I2C address assumed to be 0x27. If LCD doesn't respond, run an I2C scanner to find the actual address.
- `tone()` on STM32duino may behave differently than Arduino AVR. If buzzer is silent, verify `tone()` support for the PB8 timer channel.