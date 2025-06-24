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
    
    bool result = (strcmp(response, "+EVT:JOINED") == 0);
    
    if (result) {
        LOG_INFO("[ResponseHandler] JOIN response confirmed: %s", response);
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
    
    if (strcmp(response, "+EVT:SEND_CONFIRMED_OK") == 0) {
        LOG_INFO("[ResponseHandler] SEND response: CONFIRMED_OK");
        return RESPONSE_OK;
    }
    if (strncmp(response, "+EVT:SEND_CONFIRMED_FAILED", 25) == 0) {
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

