#pragma once
#include <STM32FreeRTOS.h>

/**
 * @brief   Lowest priority task — drives all visual output
 *          Reads ReactionEvent_t from xDisplayQueue
 *          Phase 1: LCD 1602 I2C
 *          Phase 2: ILI9488 3.5" TFT upgrade
 *          Priority: 1
 */
void vDisplayTask(void *pvParameters);