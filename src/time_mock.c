#include "time.h"

// Mock 전용 전역 변수
static uint32_t mock_current_time = 0;

// ============================================================================
// Mock 함수 (테스트용)
// ============================================================================

void TIME_Mock_SetCurrentTime(uint32_t ms)
{
    mock_current_time = ms;
}

void TIME_Mock_AdvanceTime(uint32_t ms)
{
    mock_current_time += ms;
}

void TIME_Mock_Reset(void)
{
    mock_current_time = 0;
}

// ============================================================================
// Mock 플랫폼 함수들
// ============================================================================

uint32_t TIME_Platform_GetCurrentMs(void)
{
    return mock_current_time;
}

void TIME_Platform_DelayMs(uint32_t ms)
{
    mock_current_time += ms;
} 