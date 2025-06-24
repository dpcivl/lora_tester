#ifdef TEST

#include "unity.h"
#include "uart.h"
#include "time.h"
#include "mock_logger.h"
#include "../src/uart_common.c"
#include "../src/time_common.c"
#include "../src/uart_mock.c"
#include "../src/time_mock.c"

void setUp(void)
{
    LOGGER_SendWithLevel_IgnoreAndReturn(LOGGER_STATUS_OK);
    UART_Mock_Reset();
    TIME_Mock_Reset();
}

void tearDown(void)
{
    UART_Mock_Reset();
    TIME_Mock_Reset();
}

// ============================================================================
// UART 타임아웃 수신 테스트
// ============================================================================

void test_UART_ReceiveWithTimeout_should_succeed_with_data_available(void)
{
    UART_Connect("COM1");
    UART_Mock_SetReceiveData("OK\r\n");
    
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 1000;
    
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_EQUAL(4, bytes_received);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
}

void test_UART_ReceiveWithTimeout_should_return_timeout_when_no_data(void)
{
    UART_Connect("COM1");
    
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 100;
    
    // Mock에서 지연 응답 설정 (200ms 후 응답)
    UART_Mock_SetDelayedResponse(200, "OK\r\n");
    
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    
    TEST_ASSERT_EQUAL(UART_STATUS_TIMEOUT, status);
    TEST_ASSERT_EQUAL(0, bytes_received);
}

void test_UART_ReceiveWithTimeout_should_succeed_with_delayed_data(void)
{
    UART_Connect("COM1");
    
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 500;
    
    // Mock에서 지연 응답 설정 (200ms 후 응답)
    UART_Mock_SetDelayedResponse(200, "OK\r\n");
    
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_EQUAL(4, bytes_received);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
}

void test_UART_ReceiveWithTimeout_should_fail_when_not_connected(void)
{
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 1000;
    
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
}

void test_UART_ReceiveWithTimeout_should_fail_with_invalid_parameters(void)
{
    UART_Connect("COM1");
    
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 1000;
    
    // NULL buffer
    UartStatus status = UART_ReceiveWithTimeout(NULL, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
    
    // NULL bytes_received
    status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), NULL, timeout_ms);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
    
    // Invalid buffer size
    status = UART_ReceiveWithTimeout(buffer, 0, &bytes_received, timeout_ms);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
}

// ============================================================================
// 실제 LoRa 시나리오 테스트
// ============================================================================

void test_UART_ReceiveWithTimeout_should_handle_join_response_scenario(void)
{
    UART_Connect("COM1");
    
    // Join 명령 전송
    UART_Send("AT+JOIN");
    
    // Join 응답은 10-30초 후에 올 수 있음
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 30000;  // 30초 타임아웃
    
    // Mock에서 15초 후 Join 성공 응답 설정
    UART_Mock_SetDelayedResponse(15000, "+EVT:JOINED\r\n");
    
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_EQUAL_STRING("+EVT:JOINED\r\n", buffer);
}

void test_UART_ReceiveWithTimeout_should_handle_join_timeout_scenario(void)
{
    UART_Connect("COM1");
    
    // Join 명령 전송
    UART_Send("AT+JOIN");
    
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 5000;  // 5초 타임아웃 (짧게 설정)
    
    // Mock에서 10초 후 응답 설정 (타임아웃 발생)
    UART_Mock_SetDelayedResponse(10000, "+EVT:JOINED\r\n");
    
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    
    TEST_ASSERT_EQUAL(UART_STATUS_TIMEOUT, status);
    TEST_ASSERT_EQUAL(0, bytes_received);
}

void test_UART_ReceiveWithTimeout_should_handle_multiple_responses(void)
{
    UART_Connect("COM1");
    
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 1000;
    
    // 첫 번째 응답: 즉시
    UART_Mock_SetReceiveData("OK\r\n");
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    
    // 두 번째 응답: 500ms 후
    UART_Mock_SetDelayedResponse(500, "ERROR\r\n");
    status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                    &bytes_received, timeout_ms);
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_EQUAL_STRING("ERROR\r\n", buffer);
    
    // 세 번째 응답: 타임아웃
    status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                    &bytes_received, timeout_ms);
    TEST_ASSERT_EQUAL(UART_STATUS_TIMEOUT, status);
}

// ============================================================================
// 성능 및 정확성 테스트
// ============================================================================

void test_UART_ReceiveWithTimeout_should_be_accurate_with_time(void)
{
    UART_Connect("COM1");
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 100;
    
    // Mock에서 50ms 후 응답 설정
    UART_Mock_SetDelayedResponse(50, "OK\r\n");
    
    uint32_t start_time = TIME_GetCurrentMs();
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    uint32_t end_time = TIME_GetCurrentMs();
    uint32_t elapsed = end_time - start_time;
    
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    
    // 실제 경과 시간이 50ms 근처인지 확인 (약간의 허용 오차)
    TEST_ASSERT_GREATER_THAN(45, elapsed);
    TEST_ASSERT_LESS_THAN(150, elapsed);  // 1ms 지연 때문에 더 오래 걸릴 수 있음
}

void test_UART_ReceiveWithTimeout_should_handle_zero_timeout(void)
{
    UART_Connect("COM1");
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 0;
    
    // Mock에서 지연 응답 설정
    UART_Mock_SetDelayedResponse(100, "OK\r\n");
    
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    
    TEST_ASSERT_EQUAL(UART_STATUS_TIMEOUT, status);
    TEST_ASSERT_EQUAL(0, bytes_received);
}

void test_UART_ReceiveWithTimeout_should_handle_very_long_timeout(void)
{
    UART_Connect("COM1");
    char buffer[256];
    int bytes_received;
    uint32_t timeout_ms = 60000;  // 60초
    
    // Mock에서 1초 후 응답 설정
    UART_Mock_SetDelayedResponse(1000, "OK\r\n");
    
    uint32_t start_time = TIME_GetCurrentMs();
    UartStatus status = UART_ReceiveWithTimeout(buffer, sizeof(buffer), 
                                               &bytes_received, timeout_ms);
    uint32_t end_time = TIME_GetCurrentMs();
    uint32_t elapsed = end_time - start_time;
    
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    
    // 실제 경과 시간이 1초 근처인지 확인
    TEST_ASSERT_GREATER_THAN(950, elapsed);
    TEST_ASSERT_LESS_THAN(2000, elapsed);
}

#endif // TEST 