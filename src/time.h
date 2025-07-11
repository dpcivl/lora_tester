#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// 기본 시간 함수
// ============================================================================

// 현재 시간을 밀리초 단위로 반환
uint32_t TIME_GetCurrentMs(void);

// 지정된 시간(밀리초)만큼 지연
void TIME_DelayMs(uint32_t ms);

// 타임아웃 여부를 확인
bool TIME_IsTimeout(uint32_t start_time, uint32_t timeout_ms);

// ============================================================================
// 시간 계산 함수
// ============================================================================

// 시작 시간부터 경과된 시간을 계산 (오버플로우 처리 포함)
uint32_t TIME_CalculateElapsed(uint32_t start_time);

// 시작 시간부터 남은 시간을 계산
uint32_t TIME_CalculateRemaining(uint32_t start_time, uint32_t timeout_ms);

// ============================================================================
// Mock 함수 (테스트용)
// ============================================================================

// Mock 현재 시간을 설정
void TIME_Mock_SetCurrentTime(uint32_t ms);

// Mock 시간을 진행
void TIME_Mock_AdvanceTime(uint32_t ms);

// Mock 시간을 초기화
void TIME_Mock_Reset(void);

// ============================================================================
// 플랫폼별 구현 함수 (내부용)
// ============================================================================

// 플랫폼별 현재 시간 반환
uint32_t TIME_Platform_GetCurrentMs(void);

// 플랫폼별 지연 함수
void TIME_Platform_DelayMs(uint32_t ms);

#endif // TIME_H 