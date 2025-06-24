#include "CommandSender.h"
#include "uart.h"
#include "logger.h"
#include <stddef.h>

void CommandSender_Send(const char* command)
{
    if (command != NULL) {
        LOG_DEBUG("[CommandSender] Sending command: %s", command);
        UartStatus status = UART_Send(command);
        
        if (status == UART_STATUS_OK) {
            LOG_DEBUG("[CommandSender] Command sent successfully: %s", command);
        } else {
            LOG_ERROR("[CommandSender] Failed to send command: %s (status: %d)", command, status);
        }
    } else {
        LOG_WARN("[CommandSender] Attempted to send NULL command");
    }
}
