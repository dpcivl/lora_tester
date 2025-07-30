/**
 * @file error_codes.c
 * @brief 통일된 에러 코드 시스템 구현
 * @author Refactoring Phase 3
 * @date 2025-07-30
 */

#include "error_codes.h"
#include "system_config.h"

/**
 * @brief 에러 코드를 문자열로 변환
 * @param error_code 에러 코드
 * @return 에러 코드 설명 문자열
 */
const char* ErrorCode_ToString(ResultCode error_code)
{
    switch (error_code) {
        // 성공 코드
        case RESULT_SUCCESS:
            return "SUCCESS";
        
        // 일반 에러 코드 (1-99)
        case RESULT_ERROR_GENERIC:
            return "GENERIC_ERROR";
        case RESULT_ERROR_INVALID_PARAM:
            return "INVALID_PARAMETER";
        case RESULT_ERROR_NULL_POINTER:
            return "NULL_POINTER_ACCESS";
        case RESULT_ERROR_MEMORY_ALLOC:
            return "MEMORY_ALLOCATION_FAILED";
        case RESULT_ERROR_TIMEOUT:
            return "TIMEOUT_OCCURRED";
        case RESULT_ERROR_NOT_INITIALIZED:
            return "NOT_INITIALIZED";
        case RESULT_ERROR_ALREADY_INITIALIZED:
            return "ALREADY_INITIALIZED";
        case RESULT_ERROR_NOT_SUPPORTED:
            return "NOT_SUPPORTED";
        case RESULT_ERROR_BUSY:
            return "RESOURCE_BUSY";
        case RESULT_ERROR_NOT_READY:
            return "NOT_READY";
        
        // UART 모듈 에러 코드 (100-199)
        case RESULT_ERROR_UART_CONNECTION:
            return "UART_CONNECTION_FAILED";
        case RESULT_ERROR_UART_TRANSMISSION:
            return "UART_TRANSMISSION_FAILED";
        case RESULT_ERROR_UART_RECEPTION:
            return "UART_RECEPTION_FAILED";
        case RESULT_ERROR_UART_CONFIG:
            return "UART_CONFIG_FAILED";
        case RESULT_ERROR_UART_BUFFER_FULL:
            return "UART_BUFFER_FULL";
        case RESULT_ERROR_UART_FRAMING:
            return "UART_FRAMING_ERROR";
        case RESULT_ERROR_UART_PARITY:
            return "UART_PARITY_ERROR";
        case RESULT_ERROR_UART_OVERRUN:
            return "UART_OVERRUN_ERROR";
        
        // SD Storage 모듈 에러 코드 (200-299)
        case RESULT_ERROR_SD_NOT_READY:
            return "SD_NOT_READY";
        case RESULT_ERROR_SD_FILE_ERROR:
            return "SD_FILE_ERROR";
        case RESULT_ERROR_SD_DISK_FULL:
            return "SD_DISK_FULL";
        case RESULT_ERROR_SD_MOUNT_FAILED:
            return "SD_MOUNT_FAILED";
        case RESULT_ERROR_SD_FORMAT_FAILED:
            return "SD_FORMAT_FAILED";
        case RESULT_ERROR_SD_WRITE_PROTECTED:
            return "SD_WRITE_PROTECTED";
        case RESULT_ERROR_SD_HARDWARE_FAILED:
            return "SD_HARDWARE_FAILED";
        case RESULT_ERROR_SD_FILESYSTEM_ERROR:
            return "SD_FILESYSTEM_ERROR";
        
        // Logger 모듈 에러 코드 (300-399)
        case RESULT_ERROR_LOGGER_CONNECTION:
            return "LOGGER_CONNECTION_FAILED";
        case RESULT_ERROR_LOGGER_SEND_FAILED:
            return "LOGGER_SEND_FAILED";
        case RESULT_ERROR_LOGGER_BUFFER_FULL:
            return "LOGGER_BUFFER_FULL";
        case RESULT_ERROR_LOGGER_CONFIG:
            return "LOGGER_CONFIG_ERROR";
        case RESULT_ERROR_LOGGER_NETWORK:
            return "LOGGER_NETWORK_ERROR";
        
        // LoRa 모듈 에러 코드 (400-499)
        case RESULT_ERROR_LORA_JOIN_FAILED:
            return "LORA_JOIN_FAILED";
        case RESULT_ERROR_LORA_SEND_FAILED:
            return "LORA_SEND_FAILED";
        case RESULT_ERROR_LORA_TIMEOUT:
            return "LORA_TIMEOUT";
        case RESULT_ERROR_LORA_INVALID_RESPONSE:
            return "LORA_INVALID_RESPONSE";
        case RESULT_ERROR_LORA_NOT_JOINED:
            return "LORA_NOT_JOINED";
        case RESULT_ERROR_LORA_CONFIG_FAILED:
            return "LORA_CONFIG_FAILED";
        case RESULT_ERROR_LORA_HARDWARE_ERROR:
            return "LORA_HARDWARE_ERROR";
        
        // 네트워크 모듈 에러 코드 (500-599)
        case RESULT_ERROR_NETWORK_CONNECTION:
            return "NETWORK_CONNECTION_FAILED";
        case RESULT_ERROR_NETWORK_SEND:
            return "NETWORK_SEND_FAILED";
        case RESULT_ERROR_NETWORK_RECEIVE:
            return "NETWORK_RECEIVE_FAILED";
        case RESULT_ERROR_NETWORK_DNS:
            return "NETWORK_DNS_FAILED";
        case RESULT_ERROR_NETWORK_SOCKET:
            return "NETWORK_SOCKET_ERROR";
        
        // 알 수 없는 에러 코드
        default:
            if (ErrorCode_IsModuleError(error_code, RESULT_ERROR_UART_BASE)) {
                return "UART_UNKNOWN_ERROR";
            } else if (ErrorCode_IsModuleError(error_code, RESULT_ERROR_SD_BASE)) {
                return "SD_UNKNOWN_ERROR";
            } else if (ErrorCode_IsModuleError(error_code, RESULT_ERROR_LOGGER_BASE)) {
                return "LOGGER_UNKNOWN_ERROR";
            } else if (ErrorCode_IsModuleError(error_code, RESULT_ERROR_LORA_BASE)) {
                return "LORA_UNKNOWN_ERROR";
            } else if (ErrorCode_IsModuleError(error_code, RESULT_ERROR_NETWORK_BASE)) {
                return "NETWORK_UNKNOWN_ERROR";
            } else {
                return "UNKNOWN_ERROR";
            }
    }
}