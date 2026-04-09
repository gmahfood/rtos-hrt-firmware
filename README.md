# rtos-hrt-firmware

[![PlatformIO](https://img.shields.io/badge/PlatformIO-STM32-orange?logo=platformio)](https://platformio.org/)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-11.1.0-green)](https://freertos.org/)
[![Board](https://img.shields.io/badge/Board-STM32F411%20Black%20Pill-blue)](https://stm32-base.org/boards/STM32F411CEU6-WeAct-Black-Pill-V2.0)
[![PCBWay](https://img.shields.io/badge/PCB-PCBWay-brightgreen)](https://www.pcbway.com/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey)](LICENSE)

> A multi-task human reaction timer built on the STM32F411 Black Pill with FreeRTOS. Three concurrent tasks communicate via queues to deliver microsecond-accurate reaction timing, RGB LED stimulus output, piezo audio feedback, and LCD 1602 display output — with a 3.5" ILI9488 TFT upgrade planned for Phase 2.

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

`rtos-hrt-firmware` measures human reaction time with microsecond precision using a real-time multi-task architecture built on FreeRTOS. A randomized stimulus interval fires an RGB LED; the player responds with a button press. Three isolated FreeRTOS tasks handle input capture, game control (stimulus generation, scoring, and state management), and display output, communicating exclusively through queues.

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
| SW1 | Push button | React button | 1 | SunFounder Inventor Kit |
| SW2 | Push button | Start button | 1 | SunFounder Inventor Kit |
| RV1 | Potentiometer | 10k ohm | 1 | SunFounder Inventor Kit |
| R1-R3 | Current limiting resistors | 220 ohm | 1 | SunFounder Inventor Kit |
| J1 | DC barrel jack | 5.5/2.1mm | 3 PCB | TBD |
| U2 | Voltage regulator | AMS1117-3.3 (SOT-223) | 3 PCB | TBD |
| D1 | Schottky diode | SS14 | 3 PCB | TBD |
| C1 | Electrolytic cap | 100uF / 25V | 3 PCB | TBD |
| C2-C4 | Ceramic bypass cap | 100nF | 3 PCB | TBD |

---

## FreeRTOS architecture

Three tasks run concurrently under the FreeRTOS scheduler, communicating exclusively through queues — no direct shared state.

### Data flow

```
[Buttons]
    |
    v
vInputTask ──[ xInputQueue ]──> vGameControllerTask ──[ xDisplayQueue ]──> vDisplayTask
 (debounce)                      (state machine,                           (LCD 1602 /
                                  stimulus, scoring)                        ILI9488 TFT)
```

### Task summary

| Task | Priority | Stack | Responsibility |
| ---- | -------- | ----- | -------------- |
| `vInputTask` | 3 (highest) | 256 words | Debounce buttons, send press events to input queue |
| `vGameControllerTask` | 2 | 256 words | Game state machine, random stimulus delay, reaction scoring, buzzer feedback |
| `vDisplayTask` | 1 (lowest) | 512 words | Render game state to LCD 1602 (Phase 1) or ILI9488 TFT (Phase 2) |

### Inter-task communication

| Primitive | Type | From | To | Purpose |
| --------- | ---- | ---- | -- | ------- |
| `xInputQueue` | Queue | Input | Game Controller | Carries button press events with timestamps |
| `xDisplayQueue` | Queue | Game Controller | Display | Carries game state updates for rendering |

---

## Pin configuration

| Pin | Direction | Function |
| --- | --------- | -------- |
| PB0 | Input | React button (polled with debounce) |
| PB1 | Input | Start button (polled with debounce) |
| PB4 | Output (PWM TIM3) | RGB LED — red |
| PB5 | Output (PWM TIM3) | RGB LED — green |
| PA8 | Output (PWM TIM1) | RGB LED — blue |
| PB8 | Output | Piezo buzzer |
| PA4 | Output | SPI1 CS — TFT (Phase 2) |
| PA5 | Output | SPI1 SCK (Phase 2) |
| PA6 | Input | SPI1 MISO (Phase 2) |
| PA7 | Output | SPI1 MOSI (Phase 2) |
| PB7 | I2C SDA | LCD 1602 (I2C1) |
| PB6 | I2C SCL | LCD 1602 (I2C1) |
| PA0 | Input (ADC) | Potentiometer — difficulty scaling |

---

## Getting started

### Prerequisites

- [VS Code](https://code.visualstudio.com/) + [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- STM32F411 Black Pill
- HiLetgo ST-Link V2 + SWD cable
- SunFounder Inventor Kit components listed in BOM
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) *(optional — for Udemy course work)*

### ST-Link wiring

```
ST-Link V2    →    Black Pill
SWDIO         →    DIO
SWCLK         →    CLK
GND           →    GND
3.3V          →    3.3V
```

### Clone and build

```bash
git clone https://github.com/gmahfood/rtos-hrt-firmware.git
cd rtos-hrt-firmware
```

Open in VS Code. PlatformIO will automatically install all dependencies defined in `platformio.ini`. Connect ST-Link V2, then:

```bash
pio run                   # build
pio run --target upload   # flash via ST-Link
pio device monitor        # serial output at 115200
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
- [x] `pin_config.h` — STM32 GPIO assignments and timing constants
- [x] `shared_types.h` — FreeRTOS handles, FSM states, event struct
- [ ] `task_input.cpp` — button debounce, event queue posting
- [ ] `task_game_controller.cpp` — state machine, stimulus, scoring, buzzer
- [ ] `main.cpp` — scheduler init, task creation, queue setup
- [ ] `task_display.cpp` — LCD 1602 state rendering
- [ ] ST-Link wired and board flashing confirmed
- [ ] Breadboard wiring complete
- [ ] Serial monitor output verified
- [ ] Reaction timing accuracy validated

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