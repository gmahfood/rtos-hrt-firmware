# rtos-hrt-firmware

[![PlatformIO](https://img.shields.io/badge/PlatformIO-STM32-orange?logo=platformio)](https://platformio.org/)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-10.3.3-green)](https://freertos.org/)
[![Board](https://img.shields.io/badge/Board-STM32F411%20Black%20Pill-blue)](https://stm32-base.org/boards/STM32F411CEU6-WeAct-Black-Pill-V2.0)
[![PCBWay](https://img.shields.io/badge/PCB-PCBWay-brightgreen)](https://www.pcbway.com/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey)](LICENSE)

> A multi-task human reaction timer built on the STM32F411 Black Pill with FreeRTOS. Three concurrent tasks communicate via queues and a binary semaphore to deliver accurate reaction timing, RGB LED stimulus output, piezo audio feedback, and LCD 1602 display output — with a 3.5" ILI9488 TFT upgrade planned for Phase 2.

---

## Table of contents

- [Overview](#overview)
- [Hardware](#hardware)
- [FreeRTOS architecture](#freertos-architecture)
- [Pin configuration](#pin-configuration)
- [Getting started](#getting-started)
- [PCB design](#pcb-design)
- [3D enclosure](#3d-enclosure)
- [Roadmap](#roadmap)
- [License](#license)

---

## Overview

`rtos-hrt-firmware` measures human reaction time using a real-time multi-task architecture built on FreeRTOS. A randomized stimulus interval — scaled by a potentiometer — fires an RGB LED; the player responds with a button press captured via EXTI hardware interrupt on the STM32F411's NVIC. Three isolated FreeRTOS tasks handle input capture, game control (stimulus generation, state machine, and scoring), and display output, communicating through queues and a binary semaphore.

The react button uses an interrupt-driven path (EXTI ISR → binary semaphore → input task) for minimal-latency timestamp capture. The start button is polled with software debounce — two different input strategies in one project demonstrating when each pattern is appropriate.

The STM32F411 Black Pill runs at 100MHz with 128KB RAM and 512KB Flash — representative of the class of ARM Cortex-M4 microcontrollers used in professional embedded systems development.

This is the second project in the [Baremetal Labs](https://github.com/gmahfood) embedded systems portfolio, following [`dht11-fsm-dashboard`](https://github.com/gmahfood/dht11-fsm-dashboard).

---

## Hardware

### Microcontroller specifications

| Parameter | Value |
| --------- | ----- |
| MCU | STM32F411CEU6 |
| Core | ARM Cortex-M4 with FPU |
| Clock | 100MHz |
| Flash | 512KB |
| RAM | 128KB |
| GPIO | 36 pins |
| Timers | 8 (including 2x advanced) |
| Programmer | HiLetgo ST-Link V2 via SWD |

### Bill of materials

| Ref | Component | Value / Part | Phase | Source |
| --- | --------- | ------------ | ----- | ------ |
| U1 | Microcontroller | STM32F411 Black Pill | 1 | Amazon |
| PROG1 | Programmer / debugger | HiLetgo ST-Link V2 | 1 | Amazon |
| LCD1 | LCD display | 1602 I2C (0x27) | 1 prototype | SunFounder Inventor Kit |
| DSP1 | TFT display | 3.5" ILI9488 480x320 SPI | 2 upgrade | Amazon |
| LED1 | RGB LED | Common cathode | 1 | SunFounder Inventor Kit |
| BZ1 | Piezo buzzer | Passive 3.3V | 1 | SunFounder Inventor Kit |
| SW1 | Push button | React button (EXTI interrupt) | 1 | SunFounder Inventor Kit |
| SW2 | Push button | Start button (polled) | 1 | SunFounder Inventor Kit |
| RV1 | Potentiometer | 10k ohm | 1 | SunFounder Inventor Kit |
| R1-R3 | Current limiting resistors | 220 ohm | 1 | SunFounder Inventor Kit |
| J1 | DC barrel jack | 5.5/2.1mm | 3 PCB | TBD |
| U2 | Voltage regulator | AMS1117-3.3 (SOT-223) | 3 PCB | TBD |
| D1 | Schottky diode | SS14 | 3 PCB | TBD |
| C1 | Electrolytic cap | 100uF / 25V | 3 PCB | TBD |
| C2-C4 | Ceramic bypass cap | 100nF | 3 PCB | TBD |

---

## FreeRTOS architecture

Three tasks run concurrently under the FreeRTOS scheduler. Tasks communicate through two queues and a binary semaphore. A shared volatile game state variable is written exclusively by the Game Controller and read by the other tasks.

### Data flow

```
                      EXTI ISR (falling edge)
                            |
                    xButtonSemaphore
                            |
                            v
[Start btn]──(poll)──> vInputTask ──[ xInputQueue ]──> vGameControllerTask ──[ xDisplayQueue ]──> vDisplayTask
[React btn]──(EXTI)──>  (debounce,                      (state machine,                           (LCD 1602 /
                         timestamp)                       stimulus, scoring,                        ILI9488 TFT)
                                                          buzzer, RGB LED)
```

### Task summary

| Task | Priority | Stack | Responsibility |
| ---- | -------- | ----- | -------------- |
| `vInputTask` | 3 (highest) | 256 words | React button via EXTI semaphore, start button via polling, debounce, timestamp capture, post events to input queue |
| `vGameControllerTask` | 2 | 256 words | Game state machine, random stimulus delay, RGB LED control, reaction time computation, high score tracking, buzzer feedback, post results to display queue |
| `vDisplayTask` | 1 (lowest) | 512 words | Render game state to LCD 1602 (Phase 1) or ILI9488 TFT (Phase 2) from display queue |

### Inter-task communication

| Primitive | Type | From | To | Purpose |
| --------- | ---- | ---- | -- | ------- |
| `xInputQueue` | Queue (ButtonEventMessage_t) | Input | Game Controller | Carries button press events with timestamps |
| `xDisplayQueue` | Queue (ReactionEvent_t) | Game Controller | Display | Carries reaction results for screen rendering |
| `xButtonSemaphore` | Binary semaphore | EXTI ISR | Input | Signals react button press with zero-latency wake-up |
| `gGameState` | Volatile enum | Game Controller (write) | Input, Display (read) | Shared FSM state for task coordination |

### State machine

```
IDLE ──> WAITING ──> STIMULUS ──> RESULT ──> IDLE
              |                      |
              |                      v
              |                  HIGHSCORE ──> IDLE
              |
              v
           EARLY ──> IDLE
```

---

## Pin configuration

| Pin | Board label | Direction | Function |
| --- | ----------- | --------- | -------- |
| PB0 | B0 | Input (EXTI) | React button — hardware interrupt, zero-latency capture |
| PB1 | B1 | Input | Start button — polled with software debounce |
| PB4 | B4 | Output (PWM TIM3 CH1) | RGB LED — red |
| PB5 | B5 | Output (PWM TIM3 CH2) | RGB LED — green |
| PA8 | A8 | Output (PWM TIM1 CH1) | RGB LED — blue |
| PB8 | B8 | Output | Piezo buzzer (passive, driven by tone()) |
| PA0 | A0 | Input (ADC1 CH0) | Potentiometer — difficulty scaling (12-bit, 0-4095) |
| PB7 | B7 | I2C1 SDA | LCD 1602 (Phase 1) |
| PB6 | B6 | I2C1 SCL | LCD 1602 (Phase 1) |
| PA4 | A4 | Output | SPI1 CS — ILI9488 TFT (Phase 2) |
| PA5 | A5 | Output | SPI1 SCK — ILI9488 TFT (Phase 2) |
| PA6 | A6 | Input | SPI1 MISO — ILI9488 TFT (Phase 2) |
| PA7 | A7 | Output | SPI1 MOSI — ILI9488 TFT (Phase 2) |

---

## Getting started

### Prerequisites

- [VS Code](https://code.visualstudio.com/) + [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- STM32F411 Black Pill
- HiLetgo ST-Link V2 + SWD cable
- USB-C cable (for serial monitor via USB CDC)
- SunFounder Inventor Kit components listed in BOM
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) *(optional — for Udemy course work)*

### ST-Link wiring

```
ST-Link V2    ->    Black Pill
SWDIO         ->    SWDIO
SWCLK         ->    SWCLK
GND           ->    GND
3.3V          ->    3V3
```

### Clone and build

```bash
git clone https://github.com/gmahfood/rtos-hrt-firmware.git
cd rtos-hrt-firmware
```

Open in VS Code. PlatformIO will automatically install all dependencies defined in `platformio.ini`. Connect ST-Link V2 and USB-C cable, then:

```bash
pio run                   # build
pio run --target upload   # flash via ST-Link
pio device monitor        # serial output at 115200 (via USB-C)
```

### Hardware validation

Before running the main firmware, flash the hardware test sketch to validate each peripheral in isolation:

```bash
# Rename main.cpp temporarily, then build and flash test_hardware.cpp
# See test/test_hardware.cpp and docs/test_plan.md for details
```

---

## PCB design

> **Status:** Phase 3 — not started

Custom 2-layer integration PCB designed in EasyEDA and fabricated by [PCBWay](https://www.pcbway.com/). Replaces the breadboard prototype with a permanent soldered board featuring onboard 3.3V regulation, reverse polarity protection, SWD debug header, and labeled test points.

[![PCBWay](https://img.shields.io/badge/Fabricated%20by-PCBWay-brightgreen)](https://www.pcbway.com/)

### Board specifications

| Parameter | Value |
| --------- | ----- |
| Layers | 2 |
| Dimensions | TBD |
| Thickness | 1.6mm |
| Surface finish | HASL (lead-free) |
| Copper weight | 1oz |
| Soldermask | Green |

---

## 3D enclosure

> **Status:** Phase 4 — not started

Parametric enclosure designed in OpenSCAD / CadQuery and printed on Bambu Labs P2S. Features PCB standoffs, button cutouts, LED diffuser window, and TFT display bezel.

| Parameter | Value |
| --------- | ----- |
| Printer | Bambu Labs P2S |
| Material | PETG |
| Design tool | OpenSCAD / CadQuery |

---

## Roadmap

### Phase 1 — breadboard prototype (SunFounder kit + LCD 1602)

- [x] Repository initialized and pushed
- [x] PlatformIO project configured for STM32F411
- [x] USB CDC serial enabled for Black Pill
- [x] `pin_config.h` — STM32 GPIO assignments and timing constants
- [x] `shared_types.h` — FreeRTOS handles, FSM states, event structs
- [x] `task_input.cpp` — EXTI semaphore + polled debounce, event queue posting
- [x] `task_game_controller.cpp` — state machine, stimulus, scoring, buzzer
- [x] `main.cpp` — scheduler init, task creation, queue and semaphore setup
- [x] `task_display.cpp` — LCD 1602 state rendering
- [x] ST-Link wired and board flashing confirmed
- [x] USB CDC serial monitor output verified
- [x] Hardware test sketch and test plan created
- [x] Start button (PB1) wired and tested
- [x] React button (PB0) wired and tested — EXTI interrupt confirmed
- [x] RGB LED wired and tested — stimulus visible
- [x] Piezo buzzer wired and tested — tones audible
- [x] Potentiometer wired and tested — difficulty scaling confirmed
- [x] LCD 1602 wired and tested — all game states rendering
- [x] Breadboard wiring complete
- [x] Buttons soldered to Elegoo PCB for reliable connections
- [x] Reaction timing validated (180ms personal best)

### Phase 2 — FreeRTOS refinement + TFT display upgrade

- [ ] Queue communication verified end-to-end
- [ ] False start detection tested
- [ ] Potentiometer difficulty scaling tuned
- [ ] Buzzer sequences finalized
- [ ] Full integration test — all tasks running concurrently
- [ ] 3.5" ILI9488 TFT display integrated
- [ ] TFT display task updated with color-coded UI and score history

### Phase 3 — PCB design and fabrication

- [ ] Schematic complete in EasyEDA
- [ ] PCB layout routed with ground pour
- [ ] DRC passed (0 errors)
- [ ] Gerbers exported and uploaded to `/hardware`
- [ ] Board ordered via PCBWay
- [ ] Board received, soldered, and validated

### Phase 4 — 3D printed enclosure

- [ ] Enclosure modeled in OpenSCAD / CadQuery
- [ ] Test print on Bambu Labs P2S
- [ ] Final print with PCB and TFT fit confirmed
- [ ] STLs uploaded to `/enclosure`

### Phase 5 — content and documentation

- [ ] Wiring diagram added to `/docs`
- [ ] Demo GIF recorded and added to README
- [ ] YouTube build video published
- [ ] Instagram Reel published
- [ ] PCBWay sponsorship integrated

---

## License

MIT — see [LICENSE](LICENSE) for details.

---

<p align="center">
  Built by <a href="https://github.com/gmahfood">George Mahfood</a> ·
  <a href="https://youtube.com/@baremetal.engineer">YouTube</a> ·
  <a href="https://instagram.com/baremetal.labs">Instagram</a> ·
  Baremetal Labs
</p>