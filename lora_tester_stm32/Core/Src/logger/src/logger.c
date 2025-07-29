/*
 * logger.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Lab2
 */


#include "logger.h"
#include "../SDStorage.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static bool logger_connected = false;
static LoggerMode_t current_mode = LOGGER_MODE_TERMINAL_ONLY;
static LogLevel filter_level = LOG_LEVEL_DEBUG;  // 기본적으로 모든 레벨 허용
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
    if (message == NULL) return LOGGER_STATUS_ERROR;
    
    // 새로운 모드 시스템 사용
    switch (current_mode) {
        case LOGGER_MODE_TERMINAL_ONLY:
            return LOGGER_Platform_Send(message);
            
        case LOGGER_MODE_SD_ONLY:
            if (SDStorage_IsReady()) {
                int result = SDStorage_WriteLog(message, strlen(message));
                return (result == SDSTORAGE_OK) ? LOGGER_STATUS_OK : LOGGER_STATUS_ERROR;
            }
            return LOGGER_STATUS_ERROR;
            
        case LOGGER_MODE_DUAL:
            // 터미널 우선 출력
            LOGGER_Platform_Send(message);
            // SD 출력 (실패해도 무시)
            if (SDStorage_IsReady()) {
                SDStorage_WriteLog(message, strlen(message));
            }
            return LOGGER_STATUS_OK;
            
        default:
            return LOGGER_STATUS_ERROR;
    }
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

// Logger 제어 함수들
void LOGGER_SetFilterLevel(LogLevel min_level) {
    filter_level = min_level;
}

void LOGGER_SetMode(LoggerMode_t mode) {
    current_mode = mode;
    
    // 모드에 따른 연결 상태 설정
    if (mode == LOGGER_MODE_TERMINAL_ONLY) {
        logger_connected = true;  // 터미널은 항상 연결됨
    } else if (mode == LOGGER_MODE_SD_ONLY || mode == LOGGER_MODE_DUAL) {
        // SD 백엔드 사용 시 SDStorage 연결 상태에 따라 결정
        logger_connected = SDStorage_IsReady();
    }
}

LoggerMode_t LOGGER_GetMode(void) {
    return current_mode;
}

void LOGGER_SendFormatted(LogLevel level, const char* format, ...) {
    // 필터 레벨 체크
    if (level < filter_level) return;
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
    
    // 모드에 따른 출력 처리
    switch (current_mode) {
        case LOGGER_MODE_TERMINAL_ONLY:
            LOGGER_Platform_Send(buffer);
            break;
            
        case LOGGER_MODE_SD_ONLY:
            if (SDStorage_IsReady()) {
                SDStorage_WriteLog(buffer, strlen(buffer));
            }
            break;
            
        case LOGGER_MODE_DUAL:
            // 터미널 출력 (실시간)
            LOGGER_Platform_Send(buffer);
            // SD 출력 (에러 무시)
            if (SDStorage_IsReady()) {
                SDStorage_WriteLog(buffer, strlen(buffer));
            }
            break;
    }
}
