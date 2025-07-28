#include "ResponseHandler.h"
#include "logger.h"
#include <string.h>

bool is_response_ok(const char* response)
{
    if (response == NULL) {
        LOG_DEBUG("[ResponseHandler] is_response_ok: NULL response");
        return false;
    }
    
    LOG_DEBUG("[ResponseHandler] Checking OK response: '%s'", response);
    
    // OK 또는 OK\r\n, OK\n 등 허용
    if (strcmp(response, "OK") == 0) {
        LOG_DEBUG("[ResponseHandler] OK response confirmed");
        return true;
    }
    if (strcmp(response, "OK\r\n") == 0) {
        LOG_DEBUG("[ResponseHandler] OK response confirmed (with CRLF)");
        return true;
    }
    if (strcmp(response, "OK\n") == 0) {
        LOG_DEBUG("[ResponseHandler] OK response confirmed (with LF)");
        return true;
    }
    
    // AT+VER 버전 응답도 성공으로 간주 (RUI_로 시작하는 응답)
    if (strstr(response, "RUI_") != NULL) {
        LOG_DEBUG("[ResponseHandler] Version response confirmed: %s", response);
        return true;
    }
    
    LOG_DEBUG("[ResponseHandler] Not an OK response: '%s'", response);
    return false;
}

bool is_join_response_ok(const char* response)
{
    if (response == NULL) {
        LOG_DEBUG("[ResponseHandler] is_join_response_ok: NULL response");
        return false;
    }
    
    LOG_DEBUG("[ResponseHandler] Checking JOIN response: '%s'", response);
    
    // 개행 문자 제거하여 비교
    char clean_response[512];
    strncpy(clean_response, response, sizeof(clean_response) - 1);
    clean_response[sizeof(clean_response) - 1] = '\0';
    
    // 개행 문자 제거
    char* pos = clean_response;
    while (*pos) {
        if (*pos == '\r' || *pos == '\n') {
            *pos = '\0';
            break;
        }
        pos++;
    }
    
    bool result = (strcmp(clean_response, "+EVT:JOINED") == 0);
    
    if (result) {
        LOG_WARN("[ResponseHandler] ✅ JOIN SUCCESS: %s", response);
    } else {
        LOG_DEBUG("[ResponseHandler] Not a JOIN response: '%s'", response);
    }
    
    return result;
}

ResponseType ResponseHandler_ParseSendResponse(const char* response)
{
    if (response == NULL) {
        LOG_DEBUG("[ResponseHandler] ParseSendResponse: NULL response");
        return RESPONSE_UNKNOWN;
    }
    
    LOG_DEBUG("[ResponseHandler] Parsing SEND response: '%s'", response);
    
    if (strstr(response, "+EVT:SEND_CONFIRMED_OK") != NULL) {
        LOG_WARN("[ResponseHandler] ✅ SEND SUCCESS: CONFIRMED_OK");
        return RESPONSE_OK;
    }
    if (strstr(response, "+EVT:SEND_CONFIRMED_FAILED") != NULL) {
        LOG_WARN("[ResponseHandler] SEND response: CONFIRMED_FAILED");
        return RESPONSE_ERROR;
    }
    if (strcmp(response, "TIMEOUT") == 0) {
        LOG_WARN("[ResponseHandler] SEND response: TIMEOUT");
        return RESPONSE_TIMEOUT;
    }
    
    LOG_DEBUG("[ResponseHandler] Unknown SEND response: '%s'", response);
    return RESPONSE_UNKNOWN;
}

