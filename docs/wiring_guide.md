# Breadboard Wiring Guide — rtos-hrt-firmware

> George Mahfood | Baremetal Labs
> STM32F411 Black Pill | FreeRTOS

---

## Before you start

**Wire one peripheral at a time.** After each section, flash `test_hardware.cpp` and run the corresponding test to verify wiring before moving on. This is called incremental bring-up — it prevents the nightmare of "everything is wired and nothing works."

**Orientation:** Place the Black Pill in the breadboard with the **ST-Link header (4 pins) facing up** and the **USB-C port facing down**. The left row of pins runs from 5V (top) to VB (bottom). The right row runs from B12 (bottom) to 3V3 (top).

**Power rails:** Use the breadboard's side rails. Connect the Black Pill's **3V3** pin (left side, 3rd from top) to the **+ rail** and **G** (left side, 2nd from top) to the **- rail**. All components share this 3.3V power and ground.

**Important:** The STM32F411 is a **3.3V** chip. Do NOT connect 5V to any GPIO pin — it will damage the board. The 5V pin on the Black Pill is for powering external 5V components only.

---

## Board pin reference

```
   LEFT SIDE (top to bottom)          RIGHT SIDE (bottom to top)
   ─────────────────────────          ─────────────────────────
   5V                                 3V3
   G   ← connect to GND rail         G
   3V3 ← connect to 3.3V rail        5V
   B10                                B9
   B2                                 B8   ← Buzzer
   B1  ← Start button                B7   ← I2C1 SDA (LCD)
   B0  ← React button (EXTI)         B6   ← I2C1 SCL (LCD)
   A7                                 B5   ← RGB green
   A6                                 B4   ← RGB red
   A5                                 B3
   A4                                 A15
   A3                                 A12
   A2                                 A11
   A1                                 A10
   A0  ← Potentiometer               A9
   R   (do not use)                   A8   ← RGB blue
   C15                                B15
   C14                                B14
   C13 (onboard LED)                  B13
   VB                                 B12
```

---

## Step 0: ST-Link programmer

Wire this first so you can flash firmware.

```
ST-Link V2          Black Pill (top header)
──────────          ──────────────────────
SWDIO        →      SWDIO
SWCLK        →      SWCLK
GND          →      GND
3.3V         →      3V3
```

**Verify:** Flash the test sketch. If serial monitor shows the test menu, the programmer works.

---

## Step 1: Power rails

```
Black Pill 3V3 (left side, 3rd pin from top) ──→ breadboard + rail
Black Pill G   (left side, 2nd pin from top) ──→ breadboard - rail
```

Connect the + and - rails on both sides of the breadboard together if your breadboard has a center gap.

**Verify:** Use a multimeter to confirm 3.3V between the + and - rails.

---

## Step 2: Start button (PB1)

```
         B1 ──────┐
                   │
              ┌────┴────┐
              │  BUTTON  │
              └────┬────┘
                   │
         GND rail ─┘
```

One leg of the push button connects to **B1** (left side, 6th pin from top). The other leg connects to **GND rail**.

No external resistor needed — the firmware enables the internal pull-up resistor (`INPUT_PULLUP`). The pin reads HIGH normally and goes LOW when pressed.

**Verify:** Run Test 3 (Start button polled). Press the button when prompted.

---

## Step 3: React button (PB0)

```
         B0 ──────┐
                   │
              ┌────┴────┐
              │  BUTTON  │
              └────┬────┘
                   │
         GND rail ─┘
```

Same wiring pattern as the start button. One leg to **B0** (left side, 7th pin from top), other leg to **GND rail**.

**Verify:** Run Test 4 (React button polled) AND Test 5 (React button EXTI). Both must pass — this confirms the pin works for both `digitalRead()` and hardware interrupt.

---

## Step 4: RGB LED (PB4, PB5, PA8)

The RGB LED has 4 legs. The **longest leg** is the common cathode (ground). The other three are red, green, and blue. Hold the LED with the flat side facing you — the pin order from left to right is typically: **Red, Cathode (longest), Green, Blue**. Check your specific LED's datasheet if unsure.

```
                    220Ω
         B4 ──────/\/\/──── Red leg
                    220Ω
         B5 ──────/\/\/──── Green leg
                    220Ω
         A8 ──────/\/\/──── Blue leg

         GND rail ────────── Cathode (longest leg)
```

Each color channel needs a **220 ohm resistor** between the Black Pill pin and the LED leg. This limits current to protect both the LED and the GPIO pin.

Pin locations on the board:
- **B4** — right side, 8th pin from bottom
- **B5** — right side, 7th pin from bottom
- **A8** — right side, 13th pin from bottom (there's a gap — it's between A9 and B15)

**Verify:** Run Test 6 (RGB individual channels). You should see red, green, then blue one at a time. If a color is wrong, the wiring for that leg is swapped. Then run Test 7 (PWM fade) to confirm smooth brightness control.

---

## Step 5: Piezo buzzer (PB8)

```
         B8 ────────── + leg (longer leg or marked with +)
         GND rail ──── - leg
```

**B8** is on the right side, 4th pin from bottom. The passive buzzer has a positive and negative leg — polarity matters. If your buzzer has no markings, try it both ways.

**Verify:** Run Test 8 (Buzzer). You should hear three ascending tones. If silent, flip the buzzer legs. If still silent, `tone()` may not support this timer channel on STM32duino — we'll troubleshoot if that happens.

---

## Step 6: Potentiometer (PA0)

The potentiometer has 3 pins. The middle pin is the wiper (output).

```
         3.3V rail ──── left pin (or right — doesn't matter which end)
         A0 ─────────── middle pin (wiper)
         GND rail ───── right pin (opposite end from 3.3V)
```

**A0** is on the left side, 15th pin from top (just above the R/NRST pin).

**Important:** Connect the outer pins to **3.3V and GND**, not 5V. The STM32's ADC reference voltage is 3.3V. Using 5V would give incorrect readings and could stress the ADC input.

**Verify:** Run Test 9 (Potentiometer). Turn the knob fully in both directions. The ADC values should sweep from near 0 to near 4095. If the range is narrow or stuck, check that all three pot pins are connected and the middle pin goes to A0.

---

## Step 7: LCD 1602 I2C (PB6, PB7)

The LCD 1602 with I2C backpack has 4 pins: **GND, VCC, SDA, SCL**.

```
         LCD GND ────── GND rail
         LCD VCC ────── 5V pin on Black Pill (left side, 1st pin from top)
         LCD SDA ────── B7 (right side, 5th pin from bottom)
         LCD SCL ────── B6 (right side, 6th pin from bottom)
```

**Important:** The LCD runs on **5V** (from the Black Pill's 5V pin), but the I2C lines (SDA/SCL) are 3.3V from the STM32. This works because I2C is open-drain — the LCD's I2C backpack has pull-up resistors to its own VCC, but the communication voltage is determined by the pull-up level. Most I2C LCD modules tolerate 3.3V signals fine. If you have issues, you may need a level shifter, but try it first — it usually just works.

**If the LCD shows nothing:** The I2C address might not be 0x27. Run an I2C scanner sketch to find the actual address. Common alternatives are 0x3F and 0x20.

**Verify:** Run Test 10 (LCD). You should see "rtos-hrt TEST" on the top line and "Baremetal Labs" on the bottom line.

---

## Step 8: Final check

Run Test 11 (Combined I/O) to verify all peripherals work together without conflicts. This simulates a game cycle: green LED, buzzer tone, and LCD update in rapid sequence.

When all 11 tests pass, rename `main.cpp.bak` back to `main.cpp`, flash the real firmware, and run through the integration tests in `docs/test_plan.md`.

---

## Complete wiring summary

| Component | Board pin | Board label | Side | Position (from top) |
| --------- | --------- | ----------- | ---- | ------------------- |
| React button | PB0 | B0 | Left | 7th |
| Start button | PB1 | B1 | Left | 6th |
| Potentiometer | PA0 | A0 | Left | 15th |
| RGB red | PB4 | B4 | Right | 8th from bottom |
| RGB green | PB5 | B5 | Right | 7th from bottom |
| RGB blue | PA8 | A8 | Right | 13th from bottom |
| Buzzer | PB8 | B8 | Right | 4th from bottom |
| LCD SDA | PB7 | B7 | Right | 5th from bottom |
| LCD SCL | PB6 | B6 | Right | 6th from bottom |
| LCD VCC | 5V | 5V | Left | 1st (top) |

---

## Troubleshooting

**Nothing happens after flashing:** Check ST-Link wiring. SWDIO and SWCLK are sometimes swapped.

**Button doesn't register:** Confirm one leg goes to the Black Pill pin and the other to GND. Buttons on a breadboard bridge the center gap — make sure the legs are on the correct rows.

**LED wrong color:** The RGB LED leg order varies by manufacturer. Try swapping which resistor goes to which leg.

**Buzzer silent:** Try flipping polarity. If still silent, `tone()` may not work on PB8's timer with STM32duino — we can try a different pin.

**LCD blank but backlight on:** I2C address is probably wrong. Run an I2C scanner. Also check SDA/SCL aren't swapped.

**LCD completely dark:** Check 5V power connection. The backpack needs 5V, not 3.3V.

**ADC reads 0 or 4095 constantly:** Potentiometer middle pin isn't connected to A0, or the outer pins aren't connected to 3.3V and GND.