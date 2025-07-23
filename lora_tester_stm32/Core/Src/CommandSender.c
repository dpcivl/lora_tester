#include "CommandSender.h"
#include "uart.h"
#include "logger.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

void CommandSender_Send(const char* command)
{
    if (command != NULL) {
        int len = strlen(command);
        
        // 전송할 명령어를 명확히 로깅 (특수 문자도 표시)
        LOG_INFO("📤 TX: '%s' (%d bytes)", command, len);
        
        // 헥스 덤프도 표시 (처음 20바이트까지)
        if (len > 0) {
            char hex_dump[64] = {0};
            int dump_len = (len > 20) ? 20 : len;
            for (int i = 0; i < dump_len; i++) {
                snprintf(hex_dump + i*3, 4, "%02X ", (unsigned char)command[i]);
            }
            LOG_DEBUG("[CommandSender] Hex: %s", hex_dump);
        }
        
        UartStatus status = UART_Send(command);
        
        if (status == UART_STATUS_OK) {
            LOG_DEBUG("[CommandSender] ✓ Command sent successfully");
        } else {
            LOG_ERROR("[CommandSender] ✗ Failed to send command (status: %d)", status);
        }
    } else {
        LOG_WARN("[CommandSender] Attempted to send NULL command");
    }
}
