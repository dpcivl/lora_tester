/*
 * logger.h
 *
 *  Created on: Jun 24, 2025
 *      Author: Lab2
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 로깅 레벨 정의
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3
} LogLevel;

// 로깅 상태
typedef enum {
    LOGGER_STATUS_OK = 0,
    LOGGER_STATUS_ERROR = -1,
    LOGGER_STATUS_TIMEOUT
} LoggerStatus;

// 로깅 설정
typedef struct {
    LogLevel level;
    bool enable_timestamp;
    bool enable_network;
    char server_ip[64];
    int server_port;
} LoggerConfig;

// 플랫폼 독립적 인터페이스
LoggerStatus LOGGER_Connect(const char* server_ip, int port);
LoggerStatus LOGGER_Disconnect(void);
LoggerStatus LOGGER_Send(const char* message);
LoggerStatus LOGGER_SendWithLevel(LogLevel level, const char* message);
bool LOGGER_IsConnected(void);

// 플랫폼별 구현 함수 (내부용)
LoggerStatus LOGGER_Platform_Connect(const char* server_ip, int port);
LoggerStatus LOGGER_Platform_Disconnect(void);
LoggerStatus LOGGER_Platform_Send(const char* message);
LoggerStatus LOGGER_Platform_Configure(const LoggerConfig* config);

// Logger 모드 추가
typedef enum {
    LOGGER_MODE_TERMINAL_ONLY,
    LOGGER_MODE_SD_ONLY,
    LOGGER_MODE_DUAL
} LoggerMode_t;

// Logger 제어 함수 추가
void LOGGER_SetFilterLevel(LogLevel min_level);
void LOGGER_SetSDFilterLevel(LogLevel min_level);
void LOGGER_EnableSDLogging(bool enable);
bool LOGGER_IsSDLoggingEnabled(void);
void LOGGER_SetMode(LoggerMode_t mode);
LoggerMode_t LOGGER_GetMode(void);
void LOGGER_SendFormatted(LogLevel level, const char* format, ...);

// 비동기 SD 로깅 함수 (메인 태스크 블로킹 방지)
int LOGGER_SendToSDAsync(const char* message, size_t length);

// 편의 매크로들
#define LOG_DEBUG(fmt, ...) \
    LOGGER_SendFormatted(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    LOGGER_SendFormatted(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    LOGGER_SendFormatted(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    LOGGER_SendFormatted(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

// LoRa 전용 로깅 매크로들
#define LORA_LOG_JOIN_ATTEMPT() \
    LOG_WARN("[LoRa] 🌐 JOIN ATTEMPT started")

#define LORA_LOG_JOIN_SUCCESS() \
    LOG_WARN("[LoRa] ✅ JOIN SUCCESSFUL")

#define LORA_LOG_JOIN_FAILED(reason) \
    LOG_WARN("[LoRa] JOIN failed: %s", reason)

#define LORA_LOG_SEND_ATTEMPT(message) \
    LOG_WARN("[LoRa] 📤 SEND ATTEMPT: %s", message)

#define LORA_LOG_SEND_SUCCESS() \
    LOG_WARN("[LoRa] ✅ SEND SUCCESSFUL")

#define LORA_LOG_SEND_FAILED(reason) \
    LOG_WARN("[LoRa] SEND failed: %s", reason)

#define LORA_LOG_RETRY_ATTEMPT(attempt_num, max_retries) \
    LOG_WARN("[LoRa] Retry attempt %d/%s", attempt_num, (max_retries == 0) ? "∞" : max_retries)

#define LORA_LOG_STATE_CHANGE(from_state, to_state) \
    LOG_DEBUG("[LoRa] State change: %s -> %s", from_state, to_state)

#define LORA_LOG_ERROR_COUNT(count) \
    LOG_WARN("[LoRa] Error count: %d", count)

#define LORA_LOG_MAX_RETRIES_REACHED() \
    LOG_ERROR("[LoRa] Maximum retry count reached")

#endif // LOGGER_H