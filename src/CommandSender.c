#include "CommandSender.h"
#include "uart.h"
#include <stddef.h>

void CommandSender_Send(const char* command)
{
    if (command != NULL) {
        UART_Send(command);
    }
}
