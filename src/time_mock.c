#include "time.h"
#include <stdio.h>

// Mock 전용 전역 변수
static uint32_t mock_current_time = 0;

// ============================================================================
// Mock 함수 (테스트용)
// ============================================================================

void TIME_Mock_SetCurrentTime(uint32_t ms)
{
    mock_current_time = ms;
    printf("Mock TIME: Set current time to %u ms\n", ms);
}

void TIME_Mock_AdvanceTime(uint32_t ms)
{
    mock_current_time += ms;
    printf("Mock TIME: Advanced time by %u ms (now: %u ms)\n", ms, mock_current_time);
}

void TIME_Mock_Reset(void)
{
    mock_current_time = 0;
    printf("Mock TIME: Reset to 0 ms\n");
}

// ============================================================================
// Mock 플랫폼 함수들
// ============================================================================

uint32_t TIME_Platform_GetCurrentMs(void)
{
    printf("Mock TIME: GetCurrentMs() = %u ms\n", mock_current_time);
    return mock_current_time;
}

void TIME_Platform_DelayMs(uint32_t ms)
{
    mock_current_time += ms;
    printf("Mock TIME: DelayMs(%u) - time advanced to %u ms\n", ms, mock_current_time);
} 