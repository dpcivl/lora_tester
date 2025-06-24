#ifdef TEST

#include "unity.h"
#include "mock_uart.h"

void setUp(void)
{
    // UART_Mock_Reset();
}

void tearDown(void)
{
    // UART_Mock_Reset();
}

void test_UART_Connect_should_succeed_with_valid_port(void)
{
    UART_Connect_ExpectAndReturn("COM1", UART_STATUS_OK);
    UartStatus status = UART_Connect("COM1");
    UART_IsConnected_ExpectAndReturn(true);
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_TRUE(UART_IsConnected());
}

void test_UART_Connect_should_fail_with_NULL_port(void)
{
    UART_Connect_ExpectAndReturn(NULL, UART_STATUS_ERROR);
    UartStatus status = UART_Connect(NULL);
    UART_IsConnected_ExpectAndReturn(false);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
    TEST_ASSERT_FALSE(UART_IsConnected());
}

void test_UART_Disconnect_should_succeed_when_connected(void)
{
    UART_Connect_ExpectAndReturn("COM1", UART_STATUS_OK);
    UART_Connect("COM1");
    UART_Disconnect_ExpectAndReturn(UART_STATUS_OK);
    UartStatus status = UART_Disconnect();
    UART_IsConnected_ExpectAndReturn(false);
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_FALSE(UART_IsConnected());
}

void test_UART_Disconnect_should_succeed_when_not_connected(void)
{
    UART_Disconnect_ExpectAndReturn(UART_STATUS_OK);
    UartStatus status = UART_Disconnect();
    UART_IsConnected_ExpectAndReturn(false);
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
    TEST_ASSERT_FALSE(UART_IsConnected());
}

void test_UART_Send_should_succeed_when_connected(void)
{
    UART_Connect_ExpectAndReturn("COM1", UART_STATUS_OK);
    UART_Connect("COM1");
    UART_Send_ExpectAndReturn("AT+TEST", UART_STATUS_OK);
    UartStatus status = UART_Send("AT+TEST");
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
}

void test_UART_Send_should_fail_when_not_connected(void)
{
    UART_Send_ExpectAndReturn("AT+TEST", UART_STATUS_ERROR);
    UartStatus status = UART_Send("AT+TEST");
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
}

void test_UART_Send_should_fail_with_NULL_data(void)
{
    UART_Connect_ExpectAndReturn("COM1", UART_STATUS_OK);
    UART_Connect("COM1");
    UART_Send_ExpectAndReturn(NULL, UART_STATUS_ERROR);
    UartStatus status = UART_Send(NULL);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
}

void test_UART_Receive_should_succeed_with_data_available(void)
{
    UART_Connect_ExpectAndReturn("COM1", UART_STATUS_OK);
    UART_Connect("COM1");
    char buffer[256] = {0};
    int bytes_received = 0;
    UART_Receive_ExpectAndReturn(buffer, sizeof(buffer), &bytes_received, UART_STATUS_OK);
    UartStatus status = UART_Receive(buffer, sizeof(buffer), &bytes_received);
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
}

void test_UART_Receive_should_return_timeout_when_no_data(void)
{
    UART_Connect_ExpectAndReturn("COM1", UART_STATUS_OK);
    UART_Connect("COM1");
    char buffer[256] = {0};
    int bytes_received = 0;
    UART_Receive_ExpectAndReturn(buffer, sizeof(buffer), &bytes_received, UART_STATUS_TIMEOUT);
    UartStatus status = UART_Receive(buffer, sizeof(buffer), &bytes_received);
    TEST_ASSERT_EQUAL(UART_STATUS_TIMEOUT, status);
    TEST_ASSERT_EQUAL(0, bytes_received);
}

void test_UART_Receive_should_fail_when_not_connected(void)
{
    char buffer[256] = {0};
    int bytes_received = 0;
    UART_Receive_ExpectAndReturn(buffer, sizeof(buffer), &bytes_received, UART_STATUS_ERROR);
    UartStatus status = UART_Receive(buffer, sizeof(buffer), &bytes_received);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
}

void test_UART_Receive_should_fail_with_invalid_parameters(void)
{
    UART_Connect_ExpectAndReturn("COM1", UART_STATUS_OK);
    UART_Connect("COM1");
    char buffer[256] = {0};
    int bytes_received = 0;
    UART_Receive_ExpectAndReturn(NULL, sizeof(buffer), &bytes_received, UART_STATUS_ERROR);
    UartStatus status = UART_Receive(NULL, sizeof(buffer), &bytes_received);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
    UART_Receive_ExpectAndReturn(buffer, sizeof(buffer), NULL, UART_STATUS_ERROR);
    status = UART_Receive(buffer, sizeof(buffer), NULL);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
    UART_Receive_ExpectAndReturn(buffer, 0, &bytes_received, UART_STATUS_ERROR);
    status = UART_Receive(buffer, 0, &bytes_received);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
}

void test_UART_Configure_should_succeed_with_valid_config(void)
{
    UartConfig config = {
        .baud_rate = 9600,
        .data_bits = 7,
        .stop_bits = 2,
        .parity = 1,
        .timeout_ms = 500
    };
    UART_Configure_ExpectAndReturn(&config, UART_STATUS_OK);
    UartStatus status = UART_Configure(&config);
    TEST_ASSERT_EQUAL(UART_STATUS_OK, status);
}

void test_UART_Configure_should_fail_with_NULL_config(void)
{
    UART_Configure_ExpectAndReturn(NULL, UART_STATUS_ERROR);
    UartStatus status = UART_Configure(NULL);
    TEST_ASSERT_EQUAL(UART_STATUS_ERROR, status);
}

#endif // TEST 