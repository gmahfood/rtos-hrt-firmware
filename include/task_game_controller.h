/**
 * @file    task_game_controller.h
 * @brief   Game controller task — state machine, stimulus, scoring, buzzer
 * @author  George Mahfood | Baremetal Labs
 * @board   STM32F411 Black Pill
 */

#pragma once
#include <STM32FreeRTOS.h>

/**
 * @brief   Core game logic task — owns the entire state machine
 *
 *          Reads ButtonEventMessage_t from xInputQueue
 *          Posts ReactionEvent_t to xDisplayQueue
 *          Controls RGB LED stimulus and piezo buzzer feedback
 *          Manages RAM high score tracking
 *
 *          Priority: 2
 */
void vGameControllerTask(void *pvParameters);