#ifndef RESPONSEHANDLER_H
#define RESPONSEHANDLER_H

#include <stdbool.h>

typedef enum {
    RESPONSE_OK,
    RESPONSE_ERROR,
    RESPONSE_TIMEOUT,
    RESPONSE_UNKNOWN
} ResponseType;

bool is_response_ok(const char* response);
bool is_join_response_ok(const char* response);
ResponseType ResponseHandler_ParseSendResponse(const char* response);

// 시간 관련 함수들
bool ResponseHandler_IsTimeResponse(const char* response);
void ResponseHandler_ParseTimeResponse(const char* response);
const char* ResponseHandler_GetNetworkTime(void);
bool ResponseHandler_IsTimeSynchronized(void);

#endif // RESPONSEHANDLER_H
