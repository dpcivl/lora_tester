#ifdef TEST

#include "unity.h"
#include "time.h"

// Ceedling이 이 소스 파일들을 빌드에 포함하도록 직접 include
#include "../src/time_common.c"
#include "../src/time_mock.c"

void setUp(void)
{
    TIME_Mock_Reset();
}

void tearDown(void)
{
    TIME_Mock_Reset();
}

// ============================================================================
// 기본 시간 함수 테스트
// ============================================================================

void test_TIME_GetCurrentMs_should_return_current_time(void)
{
    // Mock 시간을 1000ms로 설정
    TIME_Mock_SetCurrentTime(1000);
    
    uint32_t current_time = TIME_GetCurrentMs();
    TEST_ASSERT_EQUAL(1000, current_time);
}

void test_TIME_GetCurrentMs_should_return_updated_time_after_advance(void)
{
    TIME_Mock_SetCurrentTime(1000);
    TIME_Mock_AdvanceTime(500);
    
    uint32_t current_time = TIME_GetCurrentMs();
    TEST_ASSERT_EQUAL(1500, current_time);
}

void test_TIME_DelayMs_should_advance_time(void)
{
    TIME_Mock_SetCurrentTime(1000);
    
    TIME_DelayMs(300);
    
    uint32_t current_time = TIME_GetCurrentMs();
    TEST_ASSERT_EQUAL(1300, current_time);
}

void test_TIME_IsTimeout_should_return_false_when_not_timeout(void)
{
    uint32_t start_time = 1000;
    uint32_t timeout_ms = 500;
    
    TIME_Mock_SetCurrentTime(1200);  // 200ms 경과
    
    bool is_timeout = TIME_IsTimeout(start_time, timeout_ms);
    TEST_ASSERT_FALSE(is_timeout);
}

void test_TIME_IsTimeout_should_return_true_when_timeout(void)
{
    uint32_t start_time = 1000;
    uint32_t timeout_ms = 500;
    
    TIME_Mock_SetCurrentTime(1600);  // 600ms 경과 (타임아웃)
    
    bool is_timeout = TIME_IsTimeout(start_time, timeout_ms);
    TEST_ASSERT_TRUE(is_timeout);
}

void test_TIME_IsTimeout_should_return_true_when_exact_timeout(void)
{
    uint32_t start_time = 1000;
    uint32_t timeout_ms = 500;
    
    TIME_Mock_SetCurrentTime(1500);  // 정확히 500ms 경과
    
    bool is_timeout = TIME_IsTimeout(start_time, timeout_ms);
    TEST_ASSERT_TRUE(is_timeout);
}

// ============================================================================
// Mock 시간 함수 테스트
// ============================================================================

void test_TIME_Mock_SetCurrentTime_should_set_current_time(void)
{
    TIME_Mock_SetCurrentTime(5000);
    
    uint32_t current_time = TIME_GetCurrentMs();
    TEST_ASSERT_EQUAL(5000, current_time);
}

void test_TIME_Mock_AdvanceTime_should_increase_current_time(void)
{
    TIME_Mock_SetCurrentTime(1000);
    TIME_Mock_AdvanceTime(750);
    
    uint32_t current_time = TIME_GetCurrentMs();
    TEST_ASSERT_EQUAL(1750, current_time);
}

void test_TIME_Mock_AdvanceTime_should_work_with_zero_advance(void)
{
    TIME_Mock_SetCurrentTime(1000);
    TIME_Mock_AdvanceTime(0);
    
    uint32_t current_time = TIME_GetCurrentMs();
    TEST_ASSERT_EQUAL(1000, current_time);
}

void test_TIME_Mock_Reset_should_reset_to_zero(void)
{
    TIME_Mock_SetCurrentTime(5000);
    TIME_Mock_AdvanceTime(1000);
    TIME_Mock_Reset();
    
    uint32_t current_time = TIME_GetCurrentMs();
    TEST_ASSERT_EQUAL(0, current_time);
}

// ============================================================================
// 시간 계산 테스트
// ============================================================================

void test_TIME_CalculateElapsed_should_return_correct_elapsed_time(void)
{
    uint32_t start_time = 1000;
    TIME_Mock_SetCurrentTime(1500);
    
    uint32_t elapsed = TIME_CalculateElapsed(start_time);
    TEST_ASSERT_EQUAL(500, elapsed);
}

void test_TIME_CalculateElapsed_should_handle_overflow(void)
{
    uint32_t start_time = 0xFFFFFF00;  // 거의 최대값
    TIME_Mock_SetCurrentTime(100);     // 오버플로우 발생
    
    uint32_t elapsed = TIME_CalculateElapsed(start_time);
    TEST_ASSERT_EQUAL(356, elapsed);   // 0xFFFFFF00 + 356 = 100 (오버플로우)
}

void test_TIME_CalculateRemaining_should_return_correct_remaining_time(void)
{
    uint32_t start_time = 1000;
    uint32_t timeout_ms = 1000;
    TIME_Mock_SetCurrentTime(1300);
    
    uint32_t remaining = TIME_CalculateRemaining(start_time, timeout_ms);
    TEST_ASSERT_EQUAL(700, remaining);
}

void test_TIME_CalculateRemaining_should_return_zero_when_timeout(void)
{
    uint32_t start_time = 1000;
    uint32_t timeout_ms = 500;
    TIME_Mock_SetCurrentTime(1600);
    
    uint32_t remaining = TIME_CalculateRemaining(start_time, timeout_ms);
    TEST_ASSERT_EQUAL(0, remaining);
}

// ============================================================================
// 실제 사용 시나리오 테스트
// ============================================================================

void test_TIME_should_work_with_uart_timeout_scenario(void)
{
    // UART 타임아웃 시나리오 시뮬레이션
    uint32_t start_time = TIME_GetCurrentMs();
    uint32_t timeout_ms = 5000;
    
    // 3초 후에 응답이 온다고 가정
    TIME_Mock_AdvanceTime(3000);
    
    bool is_timeout = TIME_IsTimeout(start_time, timeout_ms);
    TEST_ASSERT_FALSE(is_timeout);
    
    uint32_t remaining = TIME_CalculateRemaining(start_time, timeout_ms);
    TEST_ASSERT_EQUAL(2000, remaining);
}

void test_TIME_should_handle_multiple_operations(void)
{
    // 여러 시간 연산이 연속으로 일어나는 상황
    TIME_Mock_SetCurrentTime(1000);
    
    uint32_t op1_start = TIME_GetCurrentMs();
    TIME_Mock_AdvanceTime(100);
    
    uint32_t op2_start = TIME_GetCurrentMs();
    TIME_Mock_AdvanceTime(200);
    
    uint32_t op1_elapsed = TIME_CalculateElapsed(op1_start);
    uint32_t op2_elapsed = TIME_CalculateElapsed(op2_start);
    
    TEST_ASSERT_EQUAL(300, op1_elapsed);  // 100 + 200
    TEST_ASSERT_EQUAL(200, op2_elapsed);  // 200
}

void test_TIME_should_handle_edge_cases(void)
{
    // 경계값 테스트
    TIME_Mock_SetCurrentTime(0);
    TEST_ASSERT_EQUAL(0, TIME_GetCurrentMs());
    
    TIME_Mock_AdvanceTime(1);
    TEST_ASSERT_EQUAL(1, TIME_GetCurrentMs());
    
    TIME_Mock_SetCurrentTime(0xFFFFFFFF);  // 최대값
    TEST_ASSERT_EQUAL(0xFFFFFFFF, TIME_GetCurrentMs());
    
    TIME_Mock_AdvanceTime(1);
    TEST_ASSERT_EQUAL(0, TIME_GetCurrentMs());  // 오버플로우
}

#endif // TEST 