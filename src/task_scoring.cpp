/**
 * @file    task_scoring.cpp
 * @brief   Scoring and high score management task
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 *
 * @note    High score stored in RAM (Phase 1). Flash persistence deferred
 *          to Phase 2 — STM32F411 has no EEPROM, sector-erase writes add
 *          complexity without value during prototype validation.
 */

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_scoring.h"

static uint16_t highScore = 9999;

static void playTone(uint32_t freq, uint32_t duration_ms) {
    tone(PIN_BUZZER, freq, duration_ms);
    vTaskDelay(pdMS_TO_TICKS(duration_ms + 20));
}

void vScoringTask(void *pvParameters) {
    (void)pvParameters;

    ReactionEvent_t evt;

    for (;;) {
        // xQueuePeek — leaves event in queue for vDisplayTask to consume
        if (xQueuePeek(xDisplayQueue, &evt, pdMS_TO_TICKS(100)) == pdTRUE) {

            Serial.print("[scoring] reaction time: ");
            Serial.print(evt.reaction_time);
            Serial.println(" ms");

            if (evt.reaction_time < highScore) {
                highScore  = evt.reaction_time;
                gGameState = STATE_HIGHSCORE;

                Serial.print("[scoring] NEW HIGH SCORE: ");
                Serial.println(highScore);

                playTone(1047, 150);
                playTone(1319, 150);
                playTone(1568, 300);
            } else {
                gGameState = STATE_RESULT;
                playTone(880, 100);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
