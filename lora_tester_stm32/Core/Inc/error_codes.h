/**
 * @file error_codes.h
 * @brief 통일된 에러 코드 시스템
 * @author Refactoring Phase 3
 * @date 2025-07-30
 * 
 * 프로젝트 전체에서 사용할 통일된 에러 코드 정의
 * 모든 모듈은 이 에러 코드를 기반으로 일관성 있는 에러 처리 구현
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 통일된 결과 코드 타입
 * 모든 함수는 이 타입을 반환하여 일관성 확보
 */
typedef int32_t ResultCode;

// ============================================================================
// 성공 코드 (0)
// ============================================================================
#define RESULT_SUCCESS                    0

// ============================================================================
// 일반 에러 코드 (1-99)
// ============================================================================
#define RESULT_ERROR_GENERIC             -1   // 일반적인 에러
#define RESULT_ERROR_INVALID_PARAM       -2   // 잘못된 매개변수
#define RESULT_ERROR_NULL_POINTER        -3   // NULL 포인터 접근
#define RESULT_ERROR_MEMORY_ALLOC        -4   // 메모리 할당 실패
#define RESULT_ERROR_TIMEOUT             -5   // 타임아웃 발생
#define RESULT_ERROR_NOT_INITIALIZED     -6   // 초기화되지 않음
#define RESULT_ERROR_ALREADY_INITIALIZED -7   // 이미 초기화됨
#define RESULT_ERROR_NOT_SUPPORTED       -8   // 지원되지 않는 기능
#define RESULT_ERROR_BUSY                -9   // 리소스 사용 중
#define RESULT_ERROR_NOT_READY           -10  // 준비되지 않음

// ============================================================================
// UART 모듈 에러 코드 (100-199)
// ============================================================================
#define RESULT_ERROR_UART_BASE           -100
#define RESULT_ERROR_UART_CONNECTION     -101 // UART 연결 실패
#define RESULT_ERROR_UART_TRANSMISSION   -102 // UART 전송 실패
#define RESULT_ERROR_UART_RECEPTION      -103 // UART 수신 실패
#define RESULT_ERROR_UART_CONFIG         -104 // UART 설정 실패
#define RESULT_ERROR_UART_BUFFER_FULL    -105 // UART 버퍼 가득참
#define RESULT_ERROR_UART_FRAMING        -106 // UART 프레이밍 에러
#define RESULT_ERROR_UART_PARITY         -107 // UART 패리티 에러
#define RESULT_ERROR_UART_OVERRUN        -108 // UART 오버런 에러

// ============================================================================
// SD Storage 모듈 에러 코드 (200-299)
// ============================================================================
#define RESULT_ERROR_SD_BASE             -200
#define RESULT_ERROR_SD_NOT_READY        -201 // SD 카드 준비되지 않음
#define RESULT_ERROR_SD_FILE_ERROR       -202 // SD 파일 에러
#define RESULT_ERROR_SD_DISK_FULL        -203 // SD 디스크 가득함
#define RESULT_ERROR_SD_MOUNT_FAILED     -204 // SD 마운트 실패
#define RESULT_ERROR_SD_FORMAT_FAILED    -205 // SD 포맷 실패
#define RESULT_ERROR_SD_WRITE_PROTECTED  -206 // SD 쓰기 보호됨
#define RESULT_ERROR_SD_HARDWARE_FAILED  -207 // SD 하드웨어 실패
#define RESULT_ERROR_SD_FILESYSTEM_ERROR -208 // SD 파일시스템 에러

// ============================================================================
// Logger 모듈 에러 코드 (300-399)
// ============================================================================
#define RESULT_ERROR_LOGGER_BASE         -300
#define RESULT_ERROR_LOGGER_CONNECTION   -301 // Logger 연결 실패
#define RESULT_ERROR_LOGGER_SEND_FAILED  -302 // Logger 전송 실패
#define RESULT_ERROR_LOGGER_BUFFER_FULL  -303 // Logger 버퍼 가득참
#define RESULT_ERROR_LOGGER_CONFIG       -304 // Logger 설정 에러
#define RESULT_ERROR_LOGGER_NETWORK      -305 // Logger 네트워크 에러

// ============================================================================
// LoRa 모듈 에러 코드 (400-499)
// ============================================================================
#define RESULT_ERROR_LORA_BASE           -400
#define RESULT_ERROR_LORA_JOIN_FAILED    -401 // LoRa JOIN 실패
#define RESULT_ERROR_LORA_SEND_FAILED    -402 // LoRa SEND 실패
#define RESULT_ERROR_LORA_TIMEOUT        -403 // LoRa 응답 타임아웃
#define RESULT_ERROR_LORA_INVALID_RESPONSE -404 // LoRa 잘못된 응답
#define RESULT_ERROR_LORA_NOT_JOINED     -405 // LoRa 네트워크 미접속
#define RESULT_ERROR_LORA_CONFIG_FAILED  -406 // LoRa 설정 실패
#define RESULT_ERROR_LORA_HARDWARE_ERROR -407 // LoRa 하드웨어 에러

// ============================================================================
// 네트워크 모듈 에러 코드 (500-599)
// ============================================================================
#define RESULT_ERROR_NETWORK_BASE        -500
#define RESULT_ERROR_NETWORK_CONNECTION  -501 // 네트워크 연결 실패
#define RESULT_ERROR_NETWORK_SEND        -502 // 네트워크 전송 실패
#define RESULT_ERROR_NETWORK_RECEIVE     -503 // 네트워크 수신 실패
#define RESULT_ERROR_NETWORK_DNS         -504 // DNS 해결 실패
#define RESULT_ERROR_NETWORK_SOCKET      -505 // 소켓 에러

// ============================================================================
// 에러 코드 유틸리티 함수들
// ============================================================================

/**
 * @brief 에러 코드를 문자열로 변환
 * @param error_code 에러 코드
 * @return 에러 코드 설명 문자열
 */
const char* ErrorCode_ToString(ResultCode error_code);

/**
 * @brief 에러 코드가 성공인지 확인
 * @param result_code 결과 코드
 * @return true if success, false if error
 */
static inline bool ErrorCode_IsSuccess(ResultCode result_code) {
    return result_code == RESULT_SUCCESS;
}

/**
 * @brief 에러 코드가 특정 모듈의 에러인지 확인
 * @param result_code 결과 코드
 * @param module_base 모듈 베이스 코드
 * @return true if error belongs to module, false otherwise
 */
static inline bool ErrorCode_IsModuleError(ResultCode result_code, ResultCode module_base) {
    return (result_code <= module_base) && (result_code > (module_base - 100));
}

// ============================================================================
// 에러 처리 매크로들
// ============================================================================

/**
 * @brief 함수 결과 체크 후 에러 시 반환
 */
#define CHECK_RESULT(result) \
    do { \
        ResultCode _check_result = (result); \
        if (!ErrorCode_IsSuccess(_check_result)) { \
            return _check_result; \
        } \
    } while(0)

/**
 * @brief 함수 결과 체크 후 에러 시 로그와 함께 반환
 */
#define CHECK_RESULT_LOG(result, format, ...) \
    do { \
        ResultCode _check_result = (result); \
        if (!ErrorCode_IsSuccess(_check_result)) { \
            LOG_ERROR("Function failed: " format " (error: %d - %s)", \
                     ##__VA_ARGS__, _check_result, ErrorCode_ToString(_check_result)); \
            return _check_result; \
        } \
    } while(0)

/**
 * @brief NULL 포인터 체크
 */
#define CHECK_NULL_PARAM(ptr) \
    do { \
        if ((ptr) == NULL) { \
            LOG_ERROR("NULL parameter: %s", #ptr); \
            return RESULT_ERROR_NULL_POINTER; \
        } \
    } while(0)

/**
 * @brief 유효성 체크
 */
#define CHECK_VALID_PARAM(condition, param_name) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("Invalid parameter: %s", param_name); \
            return RESULT_ERROR_INVALID_PARAM; \
        } \
    } while(0)

#endif // ERROR_CODES_H