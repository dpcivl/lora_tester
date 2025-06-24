#include "uart.h"
#include <string.h>
#include <stdio.h>

// 전역 변수
static bool uart_connected = false;
static UartConfig current_config = {
    .baud_rate = UART_DEFAULT_BAUD_RATE,
    .data_bits = UART_DEFAULT_DATA_BITS,
    .stop_bits = UART_DEFAULT_STOP_BITS,
    .parity = UART_DEFAULT_PARITY,
    .timeout_ms = UART_DEFAULT_TIMEOUT_MS
};

// 공통 함수들 (테스트와 실제 빌드 모두에서 사용)
UartStatus UART_Connect(const char* port)
{
    if (port == NULL) return UART_STATUS_ERROR;
    
    UartStatus status = UART_Platform_Connect(port);
    if (status == UART_STATUS_OK) {
        uart_connected = true;
    }
    return status;
}

UartStatus UART_Disconnect(void)
{
    if (!uart_connected) return UART_STATUS_OK;
    
    UartStatus status = UART_Platform_Disconnect();
    if (status == UART_STATUS_OK) {
        uart_connected = false;
    }
    return status;
}

UartStatus UART_Send(const char* data)
{
    if (!uart_connected || data == NULL) return UART_STATUS_ERROR;
    
    return UART_Platform_Send(data);
}

UartStatus UART_Receive(char* buffer, int buffer_size, int* bytes_received)
{
    if (!uart_connected || buffer == NULL || buffer_size <= 0 || bytes_received == NULL) {
        return UART_STATUS_ERROR;
    }
    
    return UART_Platform_Receive(buffer, buffer_size, bytes_received);
}

UartStatus UART_ReceiveWithTimeout(char* buffer, int buffer_size, 
                                  int* bytes_received, uint32_t timeout_ms)
{
    if (!uart_connected || buffer == NULL || buffer_size <= 0 || bytes_received == NULL) {
        return UART_STATUS_ERROR;
    }
    
    // 타임아웃이 0이면 어떤 데이터도 읽지 않고 즉시 타임아웃 반환
    if (timeout_ms == 0) {
        *bytes_received = 0;
        if (buffer && buffer_size > 0) buffer[0] = '\0';
        return UART_STATUS_TIMEOUT;
    }
    
    // 시간 모듈이 필요하므로 extern으로 포함
    extern uint32_t TIME_GetCurrentMs(void);
    extern bool TIME_IsTimeout(uint32_t start_time, uint32_t timeout_ms);
    extern void TIME_DelayMs(uint32_t ms);
    
    uint32_t start_time = TIME_GetCurrentMs();
    
    while (!TIME_IsTimeout(start_time, timeout_ms)) {
        UartStatus status = UART_Platform_Receive(buffer, buffer_size, bytes_received);
        
        if (status == UART_STATUS_OK && *bytes_received > 0) {
            // 데이터를 받았으면 성공
            return UART_STATUS_OK;
        } else if (status == UART_STATUS_ERROR) {
            // 에러가 발생했으면 즉시 반환
            return UART_STATUS_ERROR;
        }
        // UART_STATUS_TIMEOUT인 경우 계속 대기
        // CPU 사용률을 줄이기 위해 짧은 지연 추가
        TIME_DelayMs(10);  // 10ms 지연 (1ms에서 개선)
    }
    
    // 타임아웃 발생
    *bytes_received = 0;
    return UART_STATUS_TIMEOUT;
}

UartStatus UART_Configure(const UartConfig* config)
{
    if (config == NULL) return UART_STATUS_ERROR;
    
    current_config = *config;
    
    if (uart_connected) {
        return UART_Platform_Configure(config);
    }
    
    return UART_STATUS_OK;
}

bool UART_IsConnected(void)
{
    return uart_connected;
}

// Mock 테스트를 위한 초기화 함수
void UART_Reset(void)
{
    uart_connected = false;
    current_config.baud_rate = UART_DEFAULT_BAUD_RATE;
    current_config.data_bits = UART_DEFAULT_DATA_BITS;
    current_config.stop_bits = UART_DEFAULT_STOP_BITS;
    current_config.parity = UART_DEFAULT_PARITY;
    current_config.timeout_ms = UART_DEFAULT_TIMEOUT_MS;
} 