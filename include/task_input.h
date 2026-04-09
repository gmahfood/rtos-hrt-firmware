/**
 * @file    task_input.h
 * @brief   Input capture task — button debounce and event reporting
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 */

#pragma once
#include <STM32FreeRTOS.h>

/**
 * @brief   Highest priority task — captures button presses
 *
 *          React button: EXTI ISR → xButtonSemaphore → timestamp → debounce
 *          Start button: polled with software debounce
 *          Posts ButtonEventMessage_t to xInputQueue
 *
 *          This task does NOT manage game state. It only reports
 *          what the player did and when. The Game Controller decides
 *          what those inputs mean.
 *
 *          Priority: 3
 */
void vInputTask(void *pvParameters);