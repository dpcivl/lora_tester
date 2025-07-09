/*
 * logger.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Lab2
 */


#include "logger.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static bool logger_connected = false;
static LoggerConfig current_config = {
    .level = LOG_LEVEL_INFO,
    .enable_timestamp = false,
    .enable_network = true,
    .server_ip = "",
    .server_port = 0
};

LoggerStatus LOGGER_Connect(const char* server_ip, int port) {
    if (server_ip == NULL) return LOGGER_STATUS_ERROR;
    strncpy(current_config.server_ip, server_ip, sizeof(current_config.server_ip) - 1);
    current_config.server_port = port;
    LoggerStatus status = LOGGER_Platform_Connect(server_ip, port);
    if (status == LOGGER_STATUS_OK) {
        logger_connected = true;
    }
    return status;
}

LoggerStatus LOGGER_Disconnect(void) {
    if (!logger_connected) return LOGGER_STATUS_OK;
    LoggerStatus status = LOGGER_Platform_Disconnect();
    if (status == LOGGER_STATUS_OK) {
        logger_connected = false;
    }
    return status;
}

LoggerStatus LOGGER_Send(const char* message) {
    if (!logger_connected || message == NULL) return LOGGER_STATUS_ERROR;
    return LOGGER_Platform_Send(message);
}

LoggerStatus LOGGER_SendWithLevel(LogLevel level, const char* message) {
    if (level < current_config.level) return LOGGER_STATUS_OK;
    char formatted[256];
    const char* level_str[] = {"[DEBUG]", "[INFO]", "[WARN]", "[ERROR]"};
    snprintf(formatted, sizeof(formatted), "%s %s", level_str[level], message ? message : "");
    return LOGGER_Send(formatted);
}

bool LOGGER_IsConnected(void) {
    return logger_connected;
}

void LOGGER_SendFormatted(LogLevel level, const char* format, ...) {
    if (level < current_config.level) return;
    
    char buffer[512];
    const char* level_str[] = {"[DEBUG]", "[INFO]", "[WARN]", "[ERROR]"};
    
    // 레벨 문자열 추가
    int offset = snprintf(buffer, sizeof(buffer), "%s ", level_str[level]);
    
    // 가변 인수 처리
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
    va_end(args);
    
    LOGGER_Send(buffer);
}
