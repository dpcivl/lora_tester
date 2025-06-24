#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

// UART 연결 상태
typedef enum {
    UART_STATUS_OK,
    UART_STATUS_ERROR,
    UART_STATUS_TIMEOUT
} UartStatus;

// UART 설정 구조체
typedef struct {
    int baud_rate;
    int data_bits;
    int stop_bits;
    int parity;
    int timeout_ms;
} UartConfig;

// 기본 설정
#define UART_DEFAULT_BAUD_RATE 115200
#define UART_DEFAULT_DATA_BITS 8
#define UART_DEFAULT_STOP_BITS 1
#define UART_DEFAULT_PARITY 0
#define UART_DEFAULT_TIMEOUT_MS 1000

// 플랫폼 독립적 인터페이스
UartStatus UART_Connect(const char* port);
UartStatus UART_Disconnect(void);
UartStatus UART_Send(const char* data);
UartStatus UART_Receive(char* buffer, int buffer_size, int* bytes_received);
UartStatus UART_ReceiveWithTimeout(char* buffer, int buffer_size, 
                                  int* bytes_received, uint32_t timeout_ms);
UartStatus UART_Configure(const UartConfig* config);
bool UART_IsConnected(void);

// 플랫폼별 구현 함수 (내부용)
UartStatus UART_Platform_Connect(const char* port);
UartStatus UART_Platform_Disconnect(void);
UartStatus UART_Platform_Send(const char* data);
UartStatus UART_Platform_Receive(char* buffer, int buffer_size, int* bytes_received);
UartStatus UART_Platform_Configure(const UartConfig* config);

// Mock 함수들 (테스트용)
void UART_Mock_Reset(void);
void UART_Mock_SetReceiveData(const char* data);
void UART_Mock_SetDelayedResponse(uint32_t delay_ms, const char* data);

#endif // UART_H 