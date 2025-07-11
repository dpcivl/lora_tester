/*
 * logger_platform.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Lab2
 */

#include "logger_platform.h"
#include "stm32f7xx_hal.h"
#include <string.h>

extern UART_HandleTypeDef huart1; // CubeMX가 생성한 UART1 (Virtual COM Port)

LoggerStatus LOGGER_Platform_Connect(const char* server_ip, int port) {
    (void)server_ip; (void)port;
    // STM32에서는 UART1이 이미 초기화되어 있으므로 추가 설정 불필요
    return LOGGER_STATUS_OK;
}

LoggerStatus LOGGER_Platform_Disconnect(void) {
    return LOGGER_STATUS_OK;
}

LoggerStatus LOGGER_Platform_Send(const char* message) {
    if (message == NULL) return LOGGER_STATUS_ERROR;
    
    // UART1을 통해 메시지 전송 (Virtual COM Port)
    int len = strlen(message);
    if (len > 0) {
        if (HAL_UART_Transmit(&huart1, (uint8_t*)message, len, 1000) == HAL_OK) {
            // 줄바꿈 추가
            HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
            return LOGGER_STATUS_OK;
        }
    }
    return LOGGER_STATUS_ERROR;
}

LoggerStatus LOGGER_Platform_Configure(const LoggerConfig* config) {
    (void)config;
    return LOGGER_STATUS_OK;
}
