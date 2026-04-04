# rtos-hrt-firmware

[![PlatformIO](https://img.shields.io/badge/PlatformIO-AVR-orange?logo=platformio)](https://platformio.org/)
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-11.1.0-green)](https://freertos.org/)
[![Board](https://img.shields.io/badge/Board-Arduino%20Uno%20R3-blue)](https://store.arduino.cc/products/arduino-uno-rev3)
[![PCBWay](https://img.shields.io/badge/PCB-PCBWay-brightgreen)](https://www.pcbway.com/)
[![License](https://img.shields.io/badge/License-MIT-lightgrey)](LICENSE)

> A multi-task human reaction timer built on Arduino Uno R3 with FreeRTOS. Four concurrent tasks communicate via queues and binary semaphores to deliver millisecond-accurate reaction timing, EEPROM high-score persistence, RGB LED stimulus output, piezo audio feedback, LCD 1602 state display, and 7-segment reaction time readout.

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

`rtos-hrt-firmware` measures human reaction time with millisecond precision using a real-time multi-task architecture built on FreeRTOS. A randomized stimulus interval — modulated by a potentiometer — fires an RGB LED; the player responds with a button press captured via hardware interrupt. Four isolated FreeRTOS tasks handle stimulus generation, input capture, scoring logic, and display output, communicating exclusively through queues and binary semaphores.

This is the second project in the [Baremetal Labs](https://github.com/gmahfood) embedded systems portfolio, following [`dht11-fsm-dashboard`](https://github.com/gmahfood/dht11-fsm-dashboard).

---

## Hardware

### Bill of materials

| Ref | Component | Value / Part | Source |
|-----|-----------|--------------|--------|
| U1 | Microcontroller | Arduino Uno R3 | SunFounder Inventor Kit |
| LCD1 | LCD display | 1602 I2C (0x27) | SunFounder Inventor Kit |
| SEG1 | 7-segment display | MAX7219 or direct | SunFounder Inventor Kit |
| LED1 | RGB LED | Common cathode | SunFounder Inventor Kit |
| BZ1 | Piezo buzzer | Passive 5V | SunFounder Inventor Kit |
| SW1 | Push button | Start / INT0 | SunFounder Inventor Kit |
| SW2 | Push button | React | SunFounder Inventor Kit |
| SW3 | Push button | Reset | SunFounder Inventor Kit |
| RV1 | Potentiometer | 10kΩ | SunFounder Inventor Kit |
| R1–R3 | Current limiting resistors | 220Ω | SunFounder Inventor Kit |
| J1 | DC barrel jack | 5.5/2.1mm | *(Phase 3 — PCB)* |
| U2 | Voltage regulator | AMS1117-5.0 (SOT-223) | *(Phase 3 — PCB)* |
| U3 | Voltage regulator | AMS1117-3.3 (SOT-223) | *(Phase 3 — PCB)* |
| D1 | Schottky diode | SS14 | *(Phase 3 — PCB)* |
| C1 | Electrolytic cap | 100µF / 25V | *(Phase 3 — PCB)* |
| C2–C4 | Ceramic bypass cap | 100nF | *(Phase 3 — PCB)* |

---

## FreeRTOS architecture

Four tasks run concurrently under the FreeRTOS scheduler, communicating exclusively through queues and binary semaphores — no direct shared state except the volatile FSM.

### Data flow

```
vStimulusTask ──[ xReactionQueue ]──► vInputTask
                                           │
                                   [ xDisplayQueue ]
                                      │           │
                               vScoringTask   vDisplayTask
```

### Task summary

| Task | Priority | Stack | Responsibility |
|------|----------|-------|----------------|
| `vStimulusTask` | 2 | 192B | Random delay, RGB LED stimulus, post timestamp to queue |
| `vInputTask` | 3 | 128B | ISR semaphore, debounce, capture response timestamp |
| `vScoringTask` | 2 | 192B | Compute reaction delta, EEPROM high score, buzzer tones |
| `vDisplayTask` | 1 | 256B | Drive LCD 1602 and 7-segment from display queue |

### Inter-task communication

| Primitive | Type | Between | Purpose |
|-----------|------|---------|---------|
| `xReactionQueue` | Queue | Stimulus → Input | Carries stimulus timestamp |
| `xDisplayQueue` | Queue | Input → Scoring / Display | Carries completed `ReactionEvent_t` |
| `xButtonSemaphore` | Binary semaphore | ISR → Input task | Signals button press with zero latency |
| `gGameState` | Volatile enum | All tasks | Shared FSM state (`IDLE`, `WAITING`, `STIMULUS`, `RESULT`, `EARLY`, `HIGHSCORE`) |

---

## Pin configuration

| Pin | Direction | Function |
|-----|-----------|----------|
| D2 | Input (INT0) | Start / react button — hardware interrupt |
| D3 | Output (PWM) | RGB LED — blue |
| D4 | Input | React button (polled) |
| D5 | Output (PWM) | RGB LED — green |
| D6 | Output (PWM) | RGB LED — red |
| D7 | Input | Reset button |
| D8 | Output | Piezo buzzer |
| D10 | Output | MAX7219 CS |
| D11 | Output | MAX7219 DIN (MOSI) |
| D12 | Output | MAX7219 CLK |
| A0 | Input (ADC) | Potentiometer — difficulty scaling |
| A4 | I2C SDA | LCD 1602 |
| A5 | I2C SCL | LCD 1602 |

---

## Getting started

> ⚠️ Source files are being added incrementally as the project is built. Check the roadmap below for current status.

### Prerequisites

- [VS Code](https://code.visualstudio.com/) + [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- Arduino Uno R3
- SunFounder Inventor Kit components listed in BOM
- USB-A to USB-B cable

### Clone

```bash
git clone https://github.com/gmahfood/rtos-hrt-firmware.git
cd rtos-hrt-firmware
```

Open the folder in VS Code. PlatformIO will automatically install all dependencies defined in `platformio.ini`.

---

## PCB design

> **Status:** Phase 3 — not started

Custom 2-layer integration PCB designed in EasyEDA and fabricated by [PCBWay](https://www.pcbway.com/). Replaces the breadboard prototype with a permanent, soldered board featuring onboard 5V/3.3V regulation, reverse polarity protection, and labeled test points.

[![PCBWay](https://img.shields.io/badge/Fabricated%20by-PCBWay-brightgreen)](https://www.pcbway.com/)

### Board specifications

| Parameter | Value |
|-----------|-------|
| Layers | 2 |
| Dimensions | TBD |
| Thickness | 1.6mm |
| Surface finish | HASL (lead-free) |
| Copper weight | 1oz |
| Soldermask | Green |

Gerber files and EasyEDA source will be added to `/hardware` upon completion of Phase 3.

---

## 3D enclosure

> **Status:** Phase 4 — not started

Parametric enclosure designed in OpenSCAD / CadQuery and printed on Bambu Labs P2S. Features PCB standoffs, button cutouts, LED diffuser window, and LCD bezel.

| Parameter | Value |
|-----------|-------|
| Printer | Bambu Labs P2S |
| Material | PETG |
| Design tool | OpenSCAD / CadQuery |

STL files and print profiles will be added to `/enclosure` upon completion of Phase 4.

---

## Roadmap

### Phase 1 — breadboard prototype

- [x] Repository initialized and pushed
- [x] PlatformIO project configured
- [ ] `pin_config.h` — pin assignments and timing constants
- [ ] `shared_types.h` — FreeRTOS handles, FSM states, event struct
- [ ] `main.cpp` — scheduler init, ISR, task creation
- [ ] `task_stimulus.cpp` — random delay, LED fire, queue post
- [ ] `task_input.cpp` — ISR semaphore, debounce, timestamp capture
- [ ] `task_scoring.cpp` — EEPROM high score, buzzer tones
- [ ] `task_display.cpp` — LCD 1602, 7-segment state rendering
- [ ] Breadboard wiring complete
- [ ] Serial monitor output verified
- [ ] Reaction timing accuracy validated

### Phase 2 — FreeRTOS refinement

- [ ] Queue / semaphore communication verified end-to-end
- [ ] False start detection tested
- [ ] EEPROM persistence confirmed across power cycles
- [ ] Potentiometer difficulty scaling tuned
- [ ] Buzzer sequences finalized
- [ ] Full integration test — all tasks running concurrently

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
- [ ] Final print with PCB fit confirmed
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