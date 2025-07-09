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

#endif // RESPONSEHANDLER_H
