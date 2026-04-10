/**
 * @file    task_display.h
 * @brief   Display output task — LCD 1602 I2C (Phase 1), ILI9488 TFT (Phase 2)
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 */

#pragma once
#include <STM32FreeRTOS.h>

/**
 * @brief   Lowest priority task — drives all visual output
 *
 *          Reads ReactionEvent_t from xDisplayQueue
 *          Renders game state to LCD 1602 (Phase 1) or ILI9488 TFT (Phase 2)
 *          Only reads gGameState — never writes to it
 *
 *          Priority: 1
 */
void vDisplayTask(void *pvParameters);