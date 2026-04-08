/**
 * @file    main.cpp
 * @brief   rtos-hrt-firmware entry point
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 */

#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "pin_config.h"
#include "shared_types.h"
#include "task_stimulus.h"
#include "task_input.h"
#include "task_scoring.h"
#include "task_display.h"

QueueHandle_t        xReactionQueue;
QueueHandle_t        xDisplayQueue;
SemaphoreHandle_t    xButtonSemaphore;
volatile GameState_t gGameState = STATE_IDLE;

void buttonISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    Serial.println("[rtos-hrt] booting...");

    xReactionQueue   = xQueueCreate(4, sizeof(ReactionEvent_t));
    xDisplayQueue    = xQueueCreate(4, sizeof(ReactionEvent_t));
    xButtonSemaphore = xSemaphoreCreateBinary();

    if (!xReactionQueue || !xDisplayQueue || !xButtonSemaphore) {
        Serial.println("[rtos-hrt] FATAL: failed to create RTOS primitives");
        while (true) {}
    }

    pinMode(PIN_BTN_REACT, INPUT_PULLUP);
    pinMode(PIN_BTN_START, INPUT_PULLUP);
    pinMode(PIN_BTN_RESET, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_REACT), buttonISR, FALLING);

    // Order follows data flow — Stimulus → Input → Scoring → Display
    xTaskCreate(vStimulusTask, "Stimulus", 256, NULL, 2, NULL);
    xTaskCreate(vInputTask,    "Input",    256, NULL, 3, NULL);
    xTaskCreate(vScoringTask,  "Scoring",  256, NULL, 2, NULL);
    xTaskCreate(vDisplayTask,  "Display",  512, NULL, 1, NULL);

    Serial.println("[rtos-hrt] starting scheduler...");
    vTaskStartScheduler();

    Serial.println("[rtos-hrt] FATAL: scheduler exited");
    while (true) {}
}

void loop() {}
