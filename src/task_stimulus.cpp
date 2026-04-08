#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_stimulus.h"

static void setLED(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(PIN_LED_R, r);
    analogWrite(PIN_LED_G, g);
    analogWrite(PIN_LED_B, b);
}

void vStimulusTask(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        /* Wait in IDLE until game transitions to WAITING state */
        if (gGameState != STATE_WAITING) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        uint16_t pot      = analogRead(PIN_POT);
        uint32_t maxDelay = map(pot, 0, 1023, DELAY_MIN_MS, DELAY_MAX_MS);
        uint32_t delay_ms = random(DELAY_MIN_MS, maxDelay);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));

        /* Check if player pressed early during the delay */
        if (gGameState == STATE_EARLY) {
            setLED(255, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            setLED(0, 0, 0);
            gGameState = STATE_IDLE;
            continue;
        }

        /* Fire stimulus */
        gGameState = STATE_STIMULUS;
        setLED(0, 255, 0);

        /* Build event and post to queue */
        ReactionEvent_t evt;
        evt.stimulus_ms   = millis();
        evt.response_ms   = 0;
        evt.reaction_time = 0;
        xQueueSend(xReactionQueue, &evt, pdMS_TO_TICKS(100));

        while (gGameState == STATE_STIMULUS) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        setLED(0, 0, 0);
    }
}