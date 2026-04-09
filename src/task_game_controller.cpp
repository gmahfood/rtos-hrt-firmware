/**
 * @file    task_game_controller.cpp
 * @brief   Game controller task — state machine, stimulus, scoring, buzzer
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 *
 * This task owns the entire game state machine. It combines what was
 * previously split across vStimulusTask and vScoringTask into a single
 * task, eliminating the need for an intermediate queue between them.
 *
 * Responsibilities:
 *   - Generate random stimulus delay (scaled by potentiometer)
 *   - Fire RGB LED stimulus and record timestamp
 *   - Receive button events from vInputTask via xInputQueue
 *   - Compute reaction time (response timestamp - stimulus timestamp)
 *   - Track RAM high score (flash persistence deferred to Phase 2)
 *   - Drive piezo buzzer for audio feedback
 *   - Post results to xDisplayQueue for vDisplayTask
 *
 * State flow:
 *   IDLE ──► WAITING ──► STIMULUS ──► RESULT ──► IDLE
 *                 │                               ▲
 *                 └──► EARLY (false start) ────────┘
 *                               STIMULUS ──► HIGHSCORE ──► IDLE
 *
 * @note    High score stored in RAM (Phase 1). Flash persistence deferred
 *          to Phase 2 — STM32F411 has no EEPROM, sector-erase writes add
 *          complexity without value during prototype validation.
 */

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_game_controller.h"

// ─────────────────────────────────────────────────────────────────────────────
// Local helpers — static keeps these private to this translation unit.
// No other file can call or see these functions.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief   Set RGB LED color via PWM
 * @param   r   Red intensity (0–255)
 * @param   g   Green intensity (0–255)
 * @param   b   Blue intensity (0–255)
 */
static void setLED(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(PIN_LED_R, r);
    analogWrite(PIN_LED_G, g);
    analogWrite(PIN_LED_B, b);
}

/**
 * @brief   Play a tone on the piezo buzzer with RTOS-friendly delay
 * @param   freq          Frequency in Hz
 * @param   duration_ms   Tone duration in milliseconds
 *
 * @note    The 20ms pad after the tone prevents the next tone() call
 *          from cutting off the tail of the previous one.
 */
static void playTone(uint32_t freq, uint32_t duration_ms) {
    tone(PIN_BUZZER, freq, duration_ms);
    vTaskDelay(pdMS_TO_TICKS(duration_ms + 20));
}

/**
 * @brief   Drain any stale events from xInputQueue
 *
 * Called when entering WAITING state to make sure no leftover button
 * presses from a previous round are sitting in the queue. Without this,
 * a stale REACT event could instantly trigger a false start.
 */
static void flushInputQueue(void) {
    ButtonEventMessage_t discard;
    while (xQueueReceive(xInputQueue, &discard, 0) == pdTRUE) {}
}

// ─────────────────────────────────────────────────────────────────────────────
// Game Controller task
// ─────────────────────────────────────────────────────────────────────────────

void vGameControllerTask(void *pvParameters) {
    (void)pvParameters;

    uint16_t highScore = 9999;
    ButtonEventMessage_t btnEvt;

    for (;;) {
        switch (gGameState) {

            // ─────────────────────────────────────────────────────────────
            // IDLE — waiting for the player to press START
            // ─────────────────────────────────────────────────────────────
            case STATE_IDLE:
                setLED(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            // ─────────────────────────────────────────────────────────────
            // WAITING — random pre-stimulus delay
            //
            // The potentiometer sets the upper bound of the random delay.
            // During the countdown, we check for react button presses
            // every 10ms. If one arrives, it's a false start.
            //
            // Why poll xInputQueue in a loop instead of just calling
            // vTaskDelay() for the full duration? Because we need to
            // catch false starts as soon as they happen — not after
            // the entire delay has elapsed.
            // ─────────────────────────────────────────────────────────────
            case STATE_WAITING: {
                flushInputQueue();

                uint16_t pot      = analogRead(PIN_POT);
                uint32_t maxDelay = map(pot, 0, ADC_MAX, DELAY_MIN_MS, DELAY_MAX_MS);
                uint32_t delay_ms = random(DELAY_MIN_MS, maxDelay);

                // Count down in 10ms increments, checking for false starts
                uint32_t elapsed = 0;
                bool falseStart  = false;

                while (elapsed < delay_ms) {
                    if (xQueueReceive(xInputQueue, &btnEvt, pdMS_TO_TICKS(10)) == pdTRUE) {
                        if (btnEvt.event == BUTTON_EVENT_REACT) {
                            falseStart = true;
                            break;
                        }
                    }
                    elapsed += 10;
                }

                if (falseStart) {
                    gGameState = STATE_EARLY;
                } else {
                    gGameState = STATE_STIMULUS;
                }
                break;
            }

            // ─────────────────────────────────────────────────────────────
            // STIMULUS — LED fires, waiting for reaction
            //
            // The green LED turns on and we record the exact timestamp.
            // Then we block on xInputQueue waiting for the react button.
            // The timeout (DELAY_MAX_MS * 2) prevents the task from
            // blocking forever if the player walks away.
            // ─────────────────────────────────────────────────────────────
            case STATE_STIMULUS: {
                setLED(0, 255, 0);
                uint32_t stimulus_ms = millis();

                Serial.print("[game] stimulus fired at ");
                Serial.print(stimulus_ms);
                Serial.println(" ms");

                // Block until react button event arrives or timeout
                if (xQueueReceive(xInputQueue, &btnEvt, pdMS_TO_TICKS(DELAY_MAX_MS * 2)) == pdTRUE
                    && btnEvt.event == BUTTON_EVENT_REACT) {

                    // Compute reaction time
                    uint32_t response_ms   = btnEvt.timestamp;
                    uint16_t reaction_time = (uint16_t)(response_ms - stimulus_ms);

                    Serial.print("[game] reaction time: ");
                    Serial.print(reaction_time);
                    Serial.println(" ms");

                    // Package result for display task
                    ReactionEvent_t result;
                    result.stimulus_ms   = stimulus_ms;
                    result.response_ms   = response_ms;
                    result.reaction_time = reaction_time;
                    xQueueSend(xDisplayQueue, &result, pdMS_TO_TICKS(100));

                    // Check high score
                    if (reaction_time < highScore) {
                        highScore  = reaction_time;
                        gGameState = STATE_HIGHSCORE;

                        Serial.print("[game] NEW HIGH SCORE: ");
                        Serial.println(highScore);

                        // Celebration: ascending three-tone sequence
                        playTone(1047, 150);    // C6
                        playTone(1319, 150);    // E6
                        playTone(1568, 300);    // G6
                    } else {
                        gGameState = STATE_RESULT;

                        // Single confirmation tone
                        playTone(880, 100);     // A5
                    }
                } else {
                    // Timeout — no response, return to idle
                    Serial.println("[game] timeout — no reaction");
                    gGameState = STATE_IDLE;
                }

                setLED(0, 0, 0);
                break;
            }

            // ─────────────────────────────────────────────────────────────
            // RESULT / HIGHSCORE — hold the display, then return to idle
            //
            // The display task is rendering the result from xDisplayQueue.
            // We just wait here for DISPLAY_HOLD_MS before resetting.
            // This replaces the old reset button — simpler UX.
            // ─────────────────────────────────────────────────────────────
            case STATE_RESULT:
            case STATE_HIGHSCORE:
                vTaskDelay(pdMS_TO_TICKS(DISPLAY_HOLD_MS));
                gGameState = STATE_IDLE;
                break;

            // ─────────────────────────────────────────────────────────────
            // EARLY — false start penalty
            //
            // Red LED flash + warning tone. Pause for 1 second so the
            // player sees the feedback, then return to idle.
            // ─────────────────────────────────────────────────────────────
            case STATE_EARLY:
                Serial.println("[game] false start!");

                setLED(255, 0, 0);
                playTone(330, 500);     // E4 — low warning tone
                vTaskDelay(pdMS_TO_TICKS(1000));
                setLED(0, 0, 0);

                gGameState = STATE_IDLE;
                break;

            default:
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
    }
}