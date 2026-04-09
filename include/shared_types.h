/**
 * @file    shared_types.h
 * @brief   Shared data types, FreeRTOS handles, and FSM state definitions
 *
 * This header is the single source for the software architecture.
 * All three tasks include this file, it defines the data structures they
 * pass between each other and the RTOS primitives they use to communicate.
 *
 * Rule: no task should reach into another task's local variables.
 * All inter-task communication goes through the primitives declared here.
 *
 * Data flow:
 *
 *   [Buttons]
 *       │
 *       ▼
 *   vInputTask ──[ xInputQueue ]──► vGameControllerTask ──[ xDisplayQueue ]──► vDisplayTask
 *   (debounce)                       (state machine,                           (LCD 1602 /
 *                                     stimulus, scoring)                        ILI9488 TFT)
 *
 *   React button also fires EXTI ISR → xButtonSemaphore → vInputTask
 *   for zero-latency timestamp capture.
 *
 * Author : George Mahfood | Baremetal Labs
 */

#pragma once

#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

// ─────────────────────────────────────────────────────────────────────────────
// Game state machine
//
// The FSM controls the flow of the game. The Game Controller task owns
// all state transitions. Other tasks read this value to know what's
// happening, but only the Game Controller writes to it.
//
//   IDLE ──► WAITING ──► STIMULUS ──► RESULT ──► IDLE
//                │                               ▲
//                └──► EARLY (false start) ────────┘
//                              STIMULUS ──► HIGHSCORE ──► IDLE
//
// volatile: this variable is read by multiple tasks and written by the
// Game Controller. Without volatile, the compiler may cache a stale
// value in a register and a task would never see the update.
// ─────────────────────────────────────────────────────────────────────────────
typedef enum {
    STATE_IDLE,       // Waiting for player to press START
    STATE_WAITING,    // Counting down the random pre-stimulus delay
    STATE_STIMULUS,   // LED is ON — waiting for reaction button press
    STATE_RESULT,     // Reaction captured — displaying time on screen
    STATE_EARLY,      // Player pressed before stimulus — false start penalty
    STATE_HIGHSCORE   // New personal best — trigger celebration sequence
} GameState_t;

// ─────────────────────────────────────────────────────────────────────────────
// Button event types
//
// The Input Task identifies which button was pressed and tags it with
// one of these values. The Game Controller uses this to decide what
// action to take based on the current game state.
// ─────────────────────────────────────────────────────────────────────────────
typedef enum {
    BUTTON_EVENT_NONE,    // No event (default / sentinel value)
    BUTTON_EVENT_START,   // Start button pressed — begin new round
    BUTTON_EVENT_REACT    // React button pressed — player response
} ButtonEvent_t;

// ─────────────────────────────────────────────────────────────────────────────
// Button event message
//
// This struct travels from the Input Task to the Game Controller via
// xInputQueue. It carries which button was pressed and a timestamp
// so the Game Controller can compute reaction time accurately.
//
// FreeRTOS copies structs by value into the queue buffer — no pointers
// are shared between tasks. After xQueueSend(), the Input Task's local
// copy and the queue's internal copy are completely independent.
//
// Size: 8 bytes (4 + 4, enum stored as uint32_t on ARM Cortex-M4)
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    ButtonEvent_t event;       // Which button was pressed
    uint32_t      timestamp;   // millis() when the press was captured
} ButtonEventMessage_t;

// ─────────────────────────────────────────────────────────────────────────────
// Reaction event
//
// This struct travels from the Game Controller to the Display Task via
// xDisplayQueue. It carries the full timing data for one reaction round
// so the display can render the result.
//
// Flow:
//   1. Game Controller records stimulus_ms when the LED fires
//   2. Input Task sends a ButtonEventMessage_t with the press timestamp
//   3. Game Controller computes reaction_time and posts this struct
//   4. Display Task receives it and renders the result on screen
//
// Size: 10 bytes (4 + 4 + 2)
// ─────────────────────────────────────────────────────────────────────────────
typedef struct {
    uint32_t stimulus_ms;    // millis() timestamp when LED fired
    uint32_t response_ms;    // millis() timestamp when button was pressed
    uint16_t reaction_time;  // delta in milliseconds (response - stimulus)
} ReactionEvent_t;

// ─────────────────────────────────────────────────────────────────────────────
// FreeRTOS handles — defined in main.cpp, declared here for shared access
//
// extern means "this variable exists somewhere else — trust me."
// The linker resolves the actual address at build time from main.cpp.
// Every task file can use these handles without redefining them.
// ─────────────────────────────────────────────────────────────────────────────
extern QueueHandle_t     xInputQueue;       // vInputTask → vGameControllerTask
extern QueueHandle_t     xDisplayQueue;     // vGameControllerTask → vDisplayTask
extern SemaphoreHandle_t xButtonSemaphore;  // EXTI ISR → vInputTask (react button)

// ─────────────────────────────────────────────────────────────────────────────
// Shared FSM state
//
// Written by the Game Controller task. Read by Input and Display tasks.
// volatile prevents the compiler from optimizing away repeated reads
// in task loops — without it, a task might cache the value in a register
// and never see state transitions from the Game Controller.
// ─────────────────────────────────────────────────────────────────────────────
extern volatile GameState_t gGameState;