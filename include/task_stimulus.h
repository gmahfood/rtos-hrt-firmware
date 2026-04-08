#pragma once
#include <STM32FreeRTOS.h>

/* 
 * @brief   Fires RGB LED stimulus after a random delay
 *          Posts stimulus timestamp to xReactionQueue
 *          Priority: 2
 */
void vStimulusTask(void *pvParameters);