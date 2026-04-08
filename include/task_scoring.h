#pragma once
#include <STM32FreeRTOS.h>

/**
 * @brief   Computes reaction delta, manages flash high score, drives buzzer
 *          Reads completed ReactionEvent_t from xDisplayQueue
 *          Priority: 2
 */
void vScoringTask(void *pvParameters);