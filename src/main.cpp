/**
 * @file    main.cpp
 * @brief   rtos-hrt-firmware entry point
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 *
 * This file initializes hardware, creates FreeRTOS primitives (queues and
 * semaphore), creates all three tasks, and starts the scheduler.
 *
 * Once vTaskStartScheduler() is called, this code never runs again.
 * FreeRTOS takes over and the three tasks run concurrently:
 *
 *   vInputTask (priority 3)          — button debounce, event reporting
 *   vGameControllerTask (priority 2) — state machine, stimulus, scoring
 *   vDisplayTask (priority 1)        — LCD 1602 / ILI9488 TFT rendering
 *
 * Inter-task communication:
 *   xInputQueue      — ButtonEventMessage_t — Input → Game Controller
 *   xDisplayQueue    — ReactionEvent_t      — Game Controller → Display
 *   xButtonSemaphore — binary semaphore     — EXTI ISR → Input Task
 */

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_input.h"
#include "task_game_controller.h"
#include "task_display.h"

// ─────────────────────────────────────────────────────────────────────────────
// FreeRTOS primitives — declared extern in shared_types.h, defined here.
// These are the actual memory allocations. Every task accesses them
// through the extern declarations in shared_types.h.
// ─────────────────────────────────────────────────────────────────────────────
QueueHandle_t        xInputQueue;
QueueHandle_t        xDisplayQueue;
SemaphoreHandle_t    xButtonSemaphore;
volatile GameState_t gGameState = STATE_IDLE;

// ─────────────────────────────────────────────────────────────────────────────
// EXTI ISR — react button (PB0)
//
// This interrupt fires on the falling edge of PB0 (button pressed to GND).
// It gives xButtonSemaphore so that vInputTask can wake up immediately
// and capture the timestamp with minimal latency.
//
// CRITICAL: ISR code must be as short as possible. No Serial.print(),
// no delays, no queue operations with blocking. xSemaphoreGiveFromISR()
// is specifically designed to be safe inside an ISR.
//
// portYIELD_FROM_ISR() tells the scheduler to immediately switch to the
// highest-priority task that was unblocked by this ISR — which is
// vInputTask at priority 3. Without this, the context switch wouldn't
// happen until the next tick interrupt.
// ─────────────────────────────────────────────────────────────────────────────
void buttonISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("[rtos-hrt] booting...");

    // ─────────────────────────────────────────────────────────────────
    // Create FreeRTOS primitives
    //
    // Queue depths of 4 give breathing room if a producer posts
    // faster than the consumer reads. In practice, these queues
    // rarely hold more than 1 item at a time.
    // ─────────────────────────────────────────────────────────────────
    xInputQueue      = xQueueCreate(4, sizeof(ButtonEventMessage_t));
    xDisplayQueue    = xQueueCreate(4, sizeof(ReactionEvent_t));
    xButtonSemaphore = xSemaphoreCreateBinary();

    if (!xInputQueue || !xDisplayQueue || !xButtonSemaphore) {
        Serial.println("[rtos-hrt] FATAL: failed to create RTOS primitives");
        while (true) {}
    }

    // ─────────────────────────────────────────────────────────────────
    // Configure GPIO
    //
    // INPUT_PULLUP: the STM32's internal pull-up resistor holds the
    // pin HIGH when the button is not pressed. Pressing the button
    // connects the pin to GND (LOW). This means pressed = LOW.
    //
    // attachInterrupt: registers buttonISR() on the falling edge of
    // PB0. On the STM32F411, PB0 maps to EXTI line 0 with its own
    // dedicated IRQ vector — no sharing with other pins.
    // ─────────────────────────────────────────────────────────────────
    pinMode(PIN_BTN_REACT, INPUT_PULLUP);
    pinMode(PIN_BTN_START, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_REACT), buttonISR, FALLING);

    // ─────────────────────────────────────────────────────────────────
    // Create tasks
    //
    // Priority determines which task runs when multiple are ready.
    // Higher number = higher priority in FreeRTOS.
    //
    //   Input (3)          — must respond to button presses first
    //   Game Controller (2) — processes events and drives game logic
    //   Display (1)        — lowest priority, updates screen last
    //
    // Stack sizes in words (1 word = 4 bytes on ARM Cortex-M4):
    //   256 words = 1024 bytes — sufficient for tasks with simple locals
    //   512 words = 2048 bytes — display needs more due to LCD library
    // ─────────────────────────────────────────────────────────────────
    xTaskCreate(vInputTask,          "Input",   256, NULL, 3, NULL);
    xTaskCreate(vGameControllerTask, "GameCtrl", 256, NULL, 2, NULL);
    xTaskCreate(vDisplayTask,        "Display",  512, NULL, 1, NULL);

    Serial.println("[rtos-hrt] starting scheduler...");
    vTaskStartScheduler();

    // If we get here, the scheduler failed to start (out of heap memory)
    Serial.println("[rtos-hrt] FATAL: scheduler exited");
    while (true) {}
}

void loop() {
    // Never reached — FreeRTOS scheduler takes over in setup()
}
