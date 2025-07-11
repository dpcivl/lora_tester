#include "uart.h"
#include <windows.h>
#include <stdio.h>

// Windows 전용 전역 변수
static HANDLE uart_handle = INVALID_HANDLE_VALUE;

UartStatus UART_Platform_Connect(const char* port)
{
    char port_name[20];
    snprintf(port_name, sizeof(port_name), "\\\\.\\%s", port);
    
    uart_handle = CreateFileA(
        port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (uart_handle == INVALID_HANDLE_VALUE) {
        return UART_STATUS_ERROR;
    }
    
    // 기본 설정 적용
    UartConfig default_config = {
        .baud_rate = UART_DEFAULT_BAUD_RATE,
        .data_bits = UART_DEFAULT_DATA_BITS,
        .stop_bits = UART_DEFAULT_STOP_BITS,
        .parity = UART_DEFAULT_PARITY,
        .timeout_ms = UART_DEFAULT_TIMEOUT_MS
    };
    
    return UART_Platform_Configure(&default_config);
}

UartStatus UART_Platform_Disconnect(void)
{
    if (uart_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(uart_handle);
        uart_handle = INVALID_HANDLE_VALUE;
    }
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Send(const char* data)
{
    if (uart_handle == INVALID_HANDLE_VALUE || data == NULL) {
        return UART_STATUS_ERROR;
    }
    
    DWORD bytes_written;
    int data_len = strlen(data);
    
    BOOL result = WriteFile(
        uart_handle,
        data,
        data_len,
        &bytes_written,
        NULL
    );
    
    if (!result || bytes_written != data_len) {
        return UART_STATUS_ERROR;
    }
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Receive(char* buffer, int buffer_size, int* bytes_received)
{
    if (uart_handle == INVALID_HANDLE_VALUE || buffer == NULL || bytes_received == NULL) {
        return UART_STATUS_ERROR;
    }
    
    DWORD bytes_read;
    BOOL result = ReadFile(
        uart_handle,
        buffer,
        buffer_size - 1,  // NULL 종료 문자 공간 확보
        &bytes_read,
        NULL
    );
    
    if (!result) {
        return UART_STATUS_ERROR;
    }
    
    // 버퍼 오버플로우 방지
    if (bytes_read >= (DWORD)buffer_size) {
        bytes_read = buffer_size - 1;
    }
    
    buffer[bytes_read] = '\0';  // NULL 종료
    *bytes_received = bytes_read;
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Configure(const UartConfig* config)
{
    if (uart_handle == INVALID_HANDLE_VALUE || config == NULL) {
        return UART_STATUS_ERROR;
    }
    
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(uart_handle, &dcb)) {
        return UART_STATUS_ERROR;
    }
    
    // 설정 적용
    dcb.BaudRate = config->baud_rate;
    dcb.ByteSize = config->data_bits;
    dcb.StopBits = (config->stop_bits == 1) ? ONESTOPBIT : TWOSTOPBITS;
    dcb.Parity = (config->parity == 0) ? NOPARITY : ODDPARITY;
    
    if (!SetCommState(uart_handle, &dcb)) {
        return UART_STATUS_ERROR;
    }
    
    // 타임아웃 설정
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = config->timeout_ms;
    timeouts.ReadTotalTimeoutConstant = config->timeout_ms;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = config->timeout_ms;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    
    if (!SetCommTimeouts(uart_handle, &timeouts)) {
        return UART_STATUS_ERROR;
    }
    
    return UART_STATUS_OK;
} 