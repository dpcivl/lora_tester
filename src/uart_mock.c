#include "uart.h"
#include <string.h>

// Mock 전용 전역 변수
static char mock_receive_buffer[1024];
static int mock_receive_index = 0;
static int mock_receive_count = 0;

// 지연 응답 관련 변수
static char delayed_response_buffer[1024];
static uint32_t delayed_response_time = 0;
static bool delayed_response_set = false;

// Mock 설정 함수들 (테스트에서 사용)
void UART_Mock_Reset(void)
{
    memset(mock_receive_buffer, 0, sizeof(mock_receive_buffer));
    mock_receive_index = 0;
    mock_receive_count = 0;
    
    // 지연 응답 초기화
    memset(delayed_response_buffer, 0, sizeof(delayed_response_buffer));
    delayed_response_time = 0;
    delayed_response_set = false;
    
    // uart_common.c의 상태 초기화
    extern void UART_Reset(void);
    UART_Reset();
}

// 지연 응답을 강제로 초기화하는 함수
void UART_Mock_ClearDelayedResponse(void)
{
    memset(delayed_response_buffer, 0, sizeof(delayed_response_buffer));
    delayed_response_time = 0;
    delayed_response_set = false;
}

void UART_Mock_SetReceiveData(const char* data)
{
    if (data != NULL) {
        strncpy(mock_receive_buffer, data, sizeof(mock_receive_buffer) - 1);
        mock_receive_buffer[sizeof(mock_receive_buffer) - 1] = '\0';
        mock_receive_index = 0;
        mock_receive_count = strlen(data);
    }
}

void UART_Mock_SetDelayedResponse(uint32_t delay_ms, const char* data)
{
    if (data != NULL) {
        strncpy(delayed_response_buffer, data, sizeof(delayed_response_buffer) - 1);
        delayed_response_buffer[sizeof(delayed_response_buffer) - 1] = '\0';
        
        // 시간 모듈이 필요하므로 extern으로 포함
        extern uint32_t TIME_GetCurrentMs(void);
        delayed_response_time = TIME_GetCurrentMs() + delay_ms;
        delayed_response_set = true;
    }
}

// Mock 플랫폼 함수들
UartStatus UART_Platform_Connect(const char* port)
{
    if (port == NULL) return UART_STATUS_ERROR;
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Disconnect(void)
{
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Send(const char* data)
{
    if (data == NULL) {
        return UART_STATUS_ERROR;
    }
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Receive(char* buffer, int buffer_size, int* bytes_received)
{
    if (buffer == NULL || bytes_received == NULL) {
        return UART_STATUS_ERROR;
    }
    
    // 지연 응답이 설정되어 있고 시간이 되었는지 확인
    if (delayed_response_set) {
        extern uint32_t TIME_GetCurrentMs(void);
        uint32_t current_time = TIME_GetCurrentMs();
        
        if (current_time >= delayed_response_time) {
            // 지연 응답 시간이 되었으므로 지연 응답을 반환
            strncpy(buffer, delayed_response_buffer, buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            *bytes_received = strlen(buffer);
            
            // 지연 응답 초기화
            delayed_response_set = false;
            
            return UART_STATUS_OK;
        }
    }
    
    // 일반적인 즉시 응답 처리
    if (mock_receive_index >= mock_receive_count) {
        *bytes_received = 0;
        return UART_STATUS_TIMEOUT;  // 데이터 없음
    }
    
    // 한 번에 한 줄씩 반환 (개행 문자까지)
    int i = 0;
    while (mock_receive_index < mock_receive_count && i < buffer_size - 1) {
        buffer[i] = mock_receive_buffer[mock_receive_index];
        mock_receive_index++;
        
        if (buffer[i] == '\n') {
            i++;
            break;
        }
        i++;
    }
    
    buffer[i] = '\0';
    *bytes_received = i;
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Configure(const UartConfig* config)
{
    if (config == NULL) return UART_STATUS_ERROR;
    
    return UART_STATUS_OK;
} 