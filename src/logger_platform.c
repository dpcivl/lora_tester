#include "logger_platform.h"

LoggerStatus LOGGER_Platform_Connect(const char* server_ip, int port) {
    (void)server_ip; (void)port;
    return LOGGER_STATUS_OK;
}

LoggerStatus LOGGER_Platform_Disconnect(void) {
    return LOGGER_STATUS_OK;
}

LoggerStatus LOGGER_Platform_Send(const char* message) {
    (void)message;
    return LOGGER_STATUS_OK;
}

LoggerStatus LOGGER_Platform_Configure(const LoggerConfig* config) {
    (void)config;
    return LOGGER_STATUS_OK;
} 