#include "time.h"

// ============================================================================
// 기본 시간 함수
// ============================================================================

uint32_t TIME_GetCurrentMs(void)
{
    return TIME_Platform_GetCurrentMs();
}

void TIME_DelayMs(uint32_t ms)
{
    TIME_Platform_DelayMs(ms);
}

bool TIME_IsTimeout(uint32_t start_time, uint32_t timeout_ms)
{
    uint32_t elapsed = TIME_CalculateElapsed(start_time);
    return elapsed >= timeout_ms;
}

// ============================================================================
// 시간 계산 함수
// ============================================================================

uint32_t TIME_CalculateElapsed(uint32_t start_time)
{
    uint32_t current_time = TIME_GetCurrentMs();
    
    // 오버플로우 처리: current_time이 start_time보다 작으면 오버플로우 발생
    if (current_time >= start_time) {
        return current_time - start_time;
    } else {
        // 오버플로우 발생: (0xFFFFFFFF - start_time + 1) + current_time
        return (0xFFFFFFFF - start_time + 1) + current_time;
    }
}

uint32_t TIME_CalculateRemaining(uint32_t start_time, uint32_t timeout_ms)
{
    uint32_t elapsed = TIME_CalculateElapsed(start_time);
    
    if (elapsed >= timeout_ms) {
        return 0;  // 타임아웃 발생
    } else {
        return timeout_ms - elapsed;
    }
} 