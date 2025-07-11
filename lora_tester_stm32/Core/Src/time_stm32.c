/*
 * time_stm32.c
 *
 *  Created on: January 2025
 *      Author: LoRa Tester
 */

#include "time.h"
#include "stm32f7xx_hal.h"

// STM32용 플랫폼 함수들
uint32_t TIME_Platform_GetCurrentMs(void)
{
    return HAL_GetTick();  // HAL_GetTick()은 1ms 단위로 시간을 반환
}

void TIME_Platform_DelayMs(uint32_t ms)
{
    HAL_Delay(ms);
} 