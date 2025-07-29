#include "ResponseHandler.h"
#include "logger.h"
#include "CommandSender.h"
#include <string.h>
#include <stdio.h>

// 전역 변수: 네트워크에서 수신한 시간 정보 저장
static char g_network_time[64] = {0};
static bool g_time_synchronized = false;

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
        LOG_WARN("[LoRa] 🌐 Network joined successfully - SD logging active");
        
        // JOIN 성공 후 시간 조회 요청 (네트워크 동기화 대기 후)
        LOG_INFO("[ResponseHandler] Requesting network time after JOIN success...");
        // 짧은 대기 후 시간 조회 (메인 루프에서 처리될 예정)
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

// 시간 응답 확인 함수
bool ResponseHandler_IsTimeResponse(const char* response)
{
    if (response == NULL) {
        return false;
    }
    
    return (strstr(response, "LTIME:") != NULL || strstr(response, "LTIME=") != NULL);
}

// 한국 시간대(UTC+9) 보정 함수
static void ConvertUTCToKST(char* time_str) {
    int hour, min, sec, month, day, year;
    
    // "01h51m37s on 07/29/2025" 형식에서 시간 추출
    if (sscanf(time_str, "%dh%dm%ds on %d/%d/%d", 
               &hour, &min, &sec, &month, &day, &year) == 6) {
        
        // 한국 시간대로 보정 (UTC+9)
        hour += 9;
        
        // 날짜 넘어가는 경우 처리
        if (hour >= 24) {
            hour -= 24;
            day += 1;
            
            // 월말 처리 (간단한 버전)
            int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
                days_in_month[1] = 29; // 윤년
            }
            
            if (day > days_in_month[month - 1]) {
                day = 1;
                month += 1;
                if (month > 12) {
                    month = 1;
                    year += 1;
                }
            }
        }
        
        // 한국 시간으로 수정된 시간 문자열 재구성
        snprintf(time_str, 64, "%02dh%02dm%02ds on %02d/%02d/%d (KST)", 
                 hour, min, sec, month, day, year);
    }
}

// 시간 응답 파싱 및 저장 함수
void ResponseHandler_ParseTimeResponse(const char* response)
{
    if (response == NULL || !ResponseHandler_IsTimeResponse(response)) {
        return;
    }
    
    LOG_DEBUG("[ResponseHandler] Parsing time response: '%s'", response);
    
    // LTIME 응답에서 시간 정보 추출 (LTIME: 또는 LTIME= 형식 모두 지원)
    const char* time_start = strstr(response, "LTIME:");
    if (time_start != NULL) {
        // "LTIME: 14h25m30s on 01/29/2025" 형태에서 시간 부분 추출
        time_start += 6; // "LTIME:" 부분 건너뛰기
    } else {
        time_start = strstr(response, "LTIME=");
        if (time_start != NULL) {
            // "AT+LTIME=00h00m28s on 01/01/19" 형태에서 시간 부분 추출
            time_start += 6; // "LTIME=" 부분 건너뛰기
        }
    }
    
    if (time_start != NULL) {
        
        // 앞쪽 공백 제거
        while (*time_start == ' ') {
            time_start++;
        }
        
        // 전역 변수에 시간 정보 저장 (개행 문자 제거)
        strncpy(g_network_time, time_start, sizeof(g_network_time) - 1);
        g_network_time[sizeof(g_network_time) - 1] = '\0';
        
        // 개행 문자 제거
        char* newline = strchr(g_network_time, '\r');
        if (newline) *newline = '\0';
        newline = strchr(g_network_time, '\n');
        if (newline) *newline = '\0';
        
        // 한국 시간대로 보정
        ConvertUTCToKST(g_network_time);
        
        g_time_synchronized = true;
        
        LOG_WARN("[LoRa] 🕐 Network time synchronized (KST): %s", g_network_time);
        LOG_WARN("[TIMESTAMP] Korean time: %s", g_network_time);
    }
}

// 현재 저장된 네트워크 시간 반환
const char* ResponseHandler_GetNetworkTime(void)
{
    if (g_time_synchronized) {
        return g_network_time;
    }
    return NULL;
}

// 시간 동기화 상태 확인
bool ResponseHandler_IsTimeSynchronized(void)
{
    return g_time_synchronized;
}

