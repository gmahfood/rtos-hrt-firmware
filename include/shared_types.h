/**
 * @file    shared_types.h
 * @brief   Shared data types, FreeRTOS handles, and FSM state definitions
 *
 * This header is the single source of truth for the software architecture.
 * All four tasks include this file — it defines the data structures they
 * pass between each other and the RTOS primitives they use to communicate.
 *
 * Rule: no task should reach into another task's local variables.
 * All inter-task communication goes through the primitives declared here.
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
// The FSM controls which task is "active" at any given moment.
// Only one state is valid at a time. Tasks check this before acting.
//
//   IDLE ──► WAITING ──► STIMULUS ──► RESULT ──► IDLE
//                │                               ▲
//                └──► EARLY (false start) ────────┘
//                              STIMULUS ──► HIGHSCORE ──► IDLE
//
// volatile: this variable is written by multiple tasks and an ISR.
// Without volatile, the compiler may cache a stale value in a register.
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
// Reaction event
//
// This struct is the unit of data that travels through FreeRTOS queues.
// FreeRTOS copies structs by value into the queue buffer — no pointers.
// Keeping this struct small (8 bytes) means minimal queue memory overhead.
//
// Flow:
//   1. vStimulusTask fills stimulus_ms and posts to xReactionQueue
//   2. vInputTask fills response_ms, computes reaction_time, posts to xDisplayQueue
//   3. vScoringTask and vDisplayTask consume from xDisplayQueue
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
extern QueueHandle_t     xReactionQueue;    // vStimulusTask → vInputTask
extern QueueHandle_t     xDisplayQueue;     // vInputTask → vScoringTask + vDisplayTask
extern SemaphoreHandle_t xButtonSemaphore;  // ISR → vInputTask (reaction button)

// Shared FSM state — written by tasks, read by all
extern volatile GameState_t gGameState;