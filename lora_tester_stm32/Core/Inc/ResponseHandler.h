#ifndef RESPONSEHANDLER_H
#define RESPONSEHANDLER_H

#include <stdbool.h>
#include "error_codes.h"

// Response 타입 (새로운 에러 코드 시스템과 호환)
typedef enum {
    RESPONSE_OK,
    RESPONSE_ERROR,
    RESPONSE_TIMEOUT,
    RESPONSE_UNKNOWN
} ResponseType;

// Response 타입을 ResultCode로 변환하는 유틸리티 함수
static inline ResultCode ResponseType_ToResultCode(ResponseType response) {
    switch (response) {
        case RESPONSE_OK:
            return RESULT_SUCCESS;
        case RESPONSE_ERROR:
            return RESULT_ERROR_LORA_SEND_FAILED;
        case RESPONSE_TIMEOUT:
            return RESULT_ERROR_LORA_TIMEOUT;
        case RESPONSE_UNKNOWN:
        default:
            return RESULT_ERROR_LORA_INVALID_RESPONSE;
    }
}

bool is_response_ok(const char* response);
bool is_join_response_ok(const char* response);
ResponseType ResponseHandler_ParseSendResponse(const char* response);

// 시간 관련 함수들
bool ResponseHandler_IsTimeResponse(const char* response);
void ResponseHandler_ParseTimeResponse(const char* response);
const char* ResponseHandler_GetNetworkTime(void);
bool ResponseHandler_IsTimeSynchronized(void);

#endif // RESPONSEHANDLER_H
