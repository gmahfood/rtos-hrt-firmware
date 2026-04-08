#pragma once
#include <STM32FreeRTOS.h>

/**
 * @brief   Highest priority task: reaction time capture
 *          Takes xButtonSemaphore from ISR, debounces, timestamps response,
 *          Detects false starts and state transitions
 *          Priority: 3
 */
void vInputTask(void *pvParameters); 