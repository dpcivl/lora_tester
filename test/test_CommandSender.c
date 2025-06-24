#ifdef TEST

#include "unity.h"
#include "mock_uart.h"

#include "CommandSender.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_CommandSender_Send_should_send_AT_command_to_UART(void)
{
    UART_Send_ExpectAndReturn("AT+NWM=1", UART_STATUS_OK);
    CommandSender_Send("AT+NWM=1");
}

#endif // TEST
