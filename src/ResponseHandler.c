#include "ResponseHandler.h"
#include <string.h>

bool is_response_ok(const char* response)
{
    if (response == NULL) return false;
    // OK 또는 OK\r\n, OK\n 등 허용
    if (strcmp(response, "OK") == 0) return true;
    if (strcmp(response, "OK\r\n") == 0) return true;
    if (strcmp(response, "OK\n") == 0) return true;
    return false;
}

bool is_join_response_ok(const char* response)
{
    if (response == NULL) return false;
    return strcmp(response, "+EVT:JOINED") == 0;
}

ResponseType ResponseHandler_ParseSendResponse(const char* response)
{
    if (response == NULL) return RESPONSE_UNKNOWN;
    
    if (strcmp(response, "+EVT:SEND_CONFIRMED_OK") == 0) return RESPONSE_OK;
    if (strncmp(response, "+EVT:SEND_CONFIRMED_FAILED", 25) == 0) return RESPONSE_ERROR;
    if (strcmp(response, "TIMEOUT") == 0) return RESPONSE_TIMEOUT;
    
    return RESPONSE_UNKNOWN;
}

