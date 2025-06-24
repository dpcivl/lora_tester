#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>

// 로깅 레벨 정의
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

// 로깅 상태
typedef enum {
    LOGGER_STATUS_OK,
    LOGGER_STATUS_ERROR,
    LOGGER_STATUS_TIMEOUT
} LoggerStatus;

// 로깅 설정
typedef struct {
    LogLevel level;
    bool enable_timestamp;
    bool enable_network;
    char server_ip[16];
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

#endif // LOGGER_H 