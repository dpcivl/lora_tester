/*
 * uart_stm32.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Lab2
 */

#include "uart.h"
#include "stm32f7xx_hal.h"
#include <string.h>

extern UART_HandleTypeDef huart6; // CubeMX가 생성

// 전역 변수
static bool uart_initialized = false;
static uint8_t rx_buffer[256];  // 수신 버퍼 (uint8_t로 변경)
static volatile bool data_received = false;
static volatile int received_bytes = 0;

// 인터럽트 콜백 함수
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == huart6.Instance) {
        data_received = true;
        received_bytes = strlen((char*)rx_buffer);
    }
}

// 인터럽트 에러 콜백
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == huart6.Instance) {
        data_received = false;
        received_bytes = 0;
    }
}

UartStatus UART_Platform_Connect(const char* port) {
    // STM32에서는 이미 HAL_UART_Init()이 실행됨
    // 인터럽트 수신 시작
    if (HAL_UART_Receive_IT(&huart6, rx_buffer, sizeof(rx_buffer)) == HAL_OK) {
        uart_initialized = true;
        data_received = false;
        received_bytes = 0;
        return UART_STATUS_OK;
    }
    return UART_STATUS_ERROR;
}

UartStatus UART_Platform_Disconnect(void) {
    // 인터럽트 수신 중지
    HAL_UART_AbortReceive_IT(&huart6);
    uart_initialized = false;
    data_received = false;
    received_bytes = 0;
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Send(const char* data) {
    if (data == NULL || !uart_initialized) return UART_STATUS_ERROR;
    
    int len = strlen(data);
    if (len == 0) return UART_STATUS_OK;
    
    if (HAL_UART_Transmit(&huart6, (uint8_t*)data, len, 1000) == HAL_OK) {
        return UART_STATUS_OK;
    } else {
        return UART_STATUS_ERROR;
    }
}

UartStatus UART_Platform_Receive(char* buffer, int buffer_size, int* bytes_received) {
    if (buffer == NULL || bytes_received == NULL || !uart_initialized) {
        return UART_STATUS_ERROR;
    }
    
    if (buffer_size <= 0) {
        *bytes_received = 0;
        return UART_STATUS_ERROR;
    }
    
    // 인터럽트로 수신된 데이터가 있는지 확인
    if (data_received && received_bytes > 0) {
        int copy_size = (received_bytes < buffer_size - 1) ? received_bytes : buffer_size - 1;
        memcpy(buffer, rx_buffer, copy_size);
        buffer[copy_size] = '\0';
        *bytes_received = copy_size;
        
        // 버퍼 초기화 및 인터럽트 재시작
        memset(rx_buffer, 0, sizeof(rx_buffer));
        data_received = false;
        received_bytes = 0;
        HAL_UART_Receive_IT(&huart6, rx_buffer, sizeof(rx_buffer));
        
        return UART_STATUS_OK;
    }
    
    // 데이터가 없으면 타임아웃
    *bytes_received = 0;
    return UART_STATUS_TIMEOUT;
}

UartStatus UART_Platform_Configure(const UartConfig* config) {
    if (config == NULL) return UART_STATUS_ERROR;
    
    // 새로운 설정 적용
    huart6.Init.BaudRate = config->baud_rate;
    huart6.Init.WordLength = (config->data_bits == 8) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    huart6.Init.StopBits = (config->stop_bits == 1) ? UART_STOPBITS_1 : UART_STOPBITS_2;
    huart6.Init.Parity = (config->parity == 0) ? UART_PARITY_NONE : 
                            (config->parity == 1) ? UART_PARITY_ODD : UART_PARITY_EVEN;
    
    // UART 재초기화
    if (HAL_UART_DeInit(&huart6) == HAL_OK && 
        HAL_UART_Init(&huart6) == HAL_OK) {
        
        // 인터럽트 수신 재시작
        if (uart_initialized) {
            HAL_UART_Receive_IT(&huart6, rx_buffer, sizeof(rx_buffer));
        }
        return UART_STATUS_OK;
    }
    
    return UART_STATUS_ERROR;
}
