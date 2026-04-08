/**
 * @file    task_display.cpp
 * @brief   Display output task — LCD 1602 I2C (Phase 1), ILI9488 TFT (Phase 2)
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 *
 * @note    Stack allocated at 512 words due to deep call chain through
 *          LiquidCrystal_I2C → Wire → HAL_I2C_Master_Transmit.
 */

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <LiquidCrystal_I2C.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_display.h"

static LiquidCrystal_I2C lcd(0x27, 16, 2);

static void lcdPrint(const char* line1, const char* line2 = "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

void vDisplayTask(void *pvParameters) {
    (void)pvParameters;

    lcd.init();
    lcd.backlight();
    lcdPrint("rtos-hrt-firmware", "Baremetal Labs");
    vTaskDelay(pdMS_TO_TICKS(2000));
    lcdPrint("Press START", "to begin...");

    GameState_t lastState = STATE_IDLE;
    ReactionEvent_t evt;

    for (;;) {
        GameState_t currentState = gGameState;

        if (currentState != lastState) {
            switch (currentState) {
                case STATE_IDLE:
                    lcdPrint("Press START", "to begin...");
                    break;

                case STATE_WAITING:
                    lcdPrint("Get ready...", "");
                    break;

                case STATE_STIMULUS:
                    lcdPrint("GO! GO! GO!", "React now!");
                    break;

                case STATE_EARLY:
                    lcdPrint("False start!", "Wait for green");
                    break;

                case STATE_RESULT:
                case STATE_HIGHSCORE:
                    if (xQueueReceive(xDisplayQueue, &evt, pdMS_TO_TICKS(50)) == pdTRUE) {
                        char buf[17];
                        if (currentState == STATE_HIGHSCORE) {
                            snprintf(buf, sizeof(buf), "BEST: %4u ms!", evt.reaction_time);
                            lcdPrint("** NEW RECORD **", buf);
                        } else {
                            snprintf(buf, sizeof(buf), "Time: %4u ms", evt.reaction_time);
                            lcdPrint(buf, "START for new");
                        }
                    }
                    break;

                default:
                    break;
            }
            lastState = currentState;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
