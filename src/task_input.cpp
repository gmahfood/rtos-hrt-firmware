/**
 * @file    task_input.cpp
 * @brief   Input capture task — highest priority
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 */

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_input.h"

void vInputTask(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        if (gGameState == STATE_IDLE) {
            if (digitalRead(PIN_BTN_START) == LOW) {
                vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
                if (digitalRead(PIN_BTN_START) == LOW) {
                    gGameState = STATE_WAITING;
                }
            }
        }

        if (digitalRead(PIN_BTN_RESET) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if (digitalRead(PIN_BTN_RESET) == LOW) {
                gGameState = STATE_IDLE;
            }
        }

        if (xSemaphoreTake(xButtonSemaphore, 0) == pdTRUE) {
            // Timestamp captured before debounce — delay would skew reaction time
            uint32_t press_ms = millis();

            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if (digitalRead(PIN_BTN_REACT) != LOW) {
                continue;
            }

            if (gGameState == STATE_WAITING) {
                gGameState = STATE_EARLY;
                continue;
            }

            if (gGameState == STATE_STIMULUS) {
                ReactionEvent_t evt;
                if (xQueueReceive(xReactionQueue, &evt, pdMS_TO_TICKS(10)) == pdTRUE) {
                    evt.response_ms   = press_ms;
                    evt.reaction_time = (uint16_t)(press_ms - evt.stimulus_ms);
                    xQueueSend(xDisplayQueue, &evt, pdMS_TO_TICKS(100));
                }
                gGameState = STATE_RESULT;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
