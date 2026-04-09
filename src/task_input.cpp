/**
 * @file    task_input.cpp
 * @brief   Input capture task — button debounce and event reporting
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 *
 * This task is the only code that touches physical button pins. It uses
 * two different input capture strategies to demonstrate both patterns:
 *
 *   React button (PB0): interrupt-driven via EXTI
 *     The hardware interrupt fires on the falling edge and gives
 *     xButtonSemaphore from the ISR. This task takes the semaphore,
 *     records the timestamp immediately (before debounce), then
 *     validates the press with a debounce delay. This gives the
 *     most accurate reaction timestamp possible.
 *
 *   Start button (PB1): polled with software debounce
 *     Each loop iteration checks the pin state. If LOW, wait 50ms
 *     and check again. Latency doesn't matter here, it's just
 *     starting a new round, not measuring time.
 *
 * Both paths produce the same output: a ButtonEventMessage_t posted
 * to xInputQueue for the Game Controller to consume.
 *
 * Design rule: this task NEVER writes to gGameState. It only reads
 * the state to decide which buttons are relevant right now. All state
 * transitions are owned by the Game Controller.
 */

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_input.h"

void vInputTask(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        // ─────────────────────────────────────────────────────────────
        // Start button — polled debounce
        //
        // Only listen for START when the game is idle. Pressing start
        // during other states should do nothing.
        // ─────────────────────────────────────────────────────────────
        if (gGameState == STATE_IDLE) {
            if (digitalRead(PIN_BTN_START) == LOW) {
                vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
                if (digitalRead(PIN_BTN_START) == LOW) {
                    ButtonEventMessage_t evt;
                    evt.event     = BUTTON_EVENT_START;
                    evt.timestamp = millis();
                    xQueueSend(xInputQueue, &evt, pdMS_TO_TICKS(100));
                }
            }
        }

        // ─────────────────────────────────────────────────────────────
        // React button — interrupt-driven via semaphore
        //
        // The ISR in main.cpp fires on the falling edge of PB0 and
        // gives xButtonSemaphore. We take it here with zero timeout
        // (non-blocking check). If the semaphore is available, a
        // press just happened.
        //
        // Timestamp is captured BEFORE the debounce delay. If we
        // waited until after debounce to call millis(), we'd add
        // 50ms of error to every reaction time measurement.
        //
        // After debounce, we verify the pin is still LOW. If it
        // bounced back HIGH, it was noise — we discard it.
        // ─────────────────────────────────────────────────────────────
        if (xSemaphoreTake(xButtonSemaphore, 0) == pdTRUE) {
            uint32_t press_ms = millis();

            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if (digitalRead(PIN_BTN_REACT) == LOW) {
                ButtonEventMessage_t evt;
                evt.event     = BUTTON_EVENT_REACT;
                evt.timestamp = press_ms;
                xQueueSend(xInputQueue, &evt, pdMS_TO_TICKS(100));
            }
        }

        // ─────────────────────────────────────────────────────────────
        // Yield to other tasks
        //
        // 10ms polling interval is fast enough to catch any human
        // button press but doesn't hog the CPU. The react button
        // doesn't depend on this rate — it's interrupt-driven.
        // This delay mainly governs how quickly START is detected.
        // ─────────────────────────────────────────────────────────────
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}