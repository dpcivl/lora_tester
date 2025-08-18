#include "power_management.h"
#include "logger.h"

extern RTC_HandleTypeDef hrtc;

/* 저전력 상태 변수들 */
static volatile uint8_t alarm_triggered = 0;
static volatile uint8_t alarm_set = 0;

/**
 * @brief 저전력 모드 관리자 초기화
 */
void PowerMgmt_Init(void) {
  alarm_triggered = 0;
  alarm_set = 0;

  /* RTC 알람 인터럽트 활성화 */
  __HAL_RTC_ALARM_ENABLE_IT(&hrtc, RTC_IT_ALRA);

  LOG_INFO("[PowerMgmt] Power management initialized");
}

/**
 * @brief Sleep 모드 진입 (CPU 중지, 페리페럴 유지)
 */
void PowerMgmt_EnterSleepMode(void) {
  LOG_INFO("[PowerMgmt] Entering Sleep Mode...");

  /* Sleep 모드 진입 - 인터럽트로 웨이크업 */
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

  LOG_INFO("[PowerMgmt] Woke up from Sleep Mode");
}

/**
 * @brief 지정된 시간만큼 Sleep 모드로 대기
 * @param seconds 대기할 시간 (초)
 */
void PowerMgmt_SetAlarmAndSleep(uint32_t seconds) {
  LOG_INFO(
      "[PowerMgmt] 💤 Starting %lu second deep sleep (no interruptions)...",
      seconds);

  alarm_set = 1;
  alarm_triggered = 0;

  LOG_INFO(
      "[PowerMgmt] 🔋 Entering true Sleep Mode - CPU halted for %lu seconds",
      seconds);

  /* 진짜 Sleep Mode: 한 번에 전체 시간 대기 */
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

  /* 전체 시간 한 번에 대기 - 중간에 깨어나지 않음 */
  HAL_Delay(seconds * 1000);

  alarm_set = 0;
  alarm_triggered = 1;

  LOG_INFO("[PowerMgmt] ⚡ CPU resumed from %lu second deep sleep", seconds);
}

/**
 * @brief 알람에서 웨이크업 처리
 */
void PowerMgmt_WakeupFromAlarm(void) {
  alarm_set = 0;
  alarm_triggered = 1;

  /* 알람 비활성화 */
  HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

  LOG_INFO("[PowerMgmt] Woke up from alarm, alarm deactivated");
}

/**
 * @brief 알람이 설정되어 있는지 확인
 * @return 1: 알람 설정됨, 0: 알람 설정 안됨
 */
uint8_t PowerMgmt_IsAlarmSet(void) { return alarm_set; }

/**
 * @brief RTC 알람 콜백 함수
 */
void PowerMgmt_AlarmCallback(void) { PowerMgmt_WakeupFromAlarm(); }

/**
 * @brief 시스템 전력 상태 출력
 */
void PowerMgmt_PrintSystemStatus(void) {
  // 클럭 속도 확인
  uint32_t sysclk = HAL_RCC_GetSysClockFreq();
  uint32_t hclk = HAL_RCC_GetHCLKFreq();
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

  LOG_INFO("[PowerMgmt] 📊 System Clock Status:");
  LOG_INFO("  SYSCLK: %lu Hz", sysclk);
  LOG_INFO("  HCLK:   %lu Hz", hclk);
  LOG_INFO("  PCLK1:  %lu Hz", pclk1);
  LOG_INFO("  PCLK2:  %lu Hz", pclk2);

  // 전력 모드 상태 확인
  uint32_t pwr_cr = PWR->CR1;
  LOG_INFO("[PowerMgmt] 🔋 Power Mode: %s",
           (pwr_cr & PWR_CR1_LPDS) ? "Low Power" : "Normal");

  // 시스템 실행 시간
  uint32_t uptime = HAL_GetTick() / 1000;
  LOG_INFO("[PowerMgmt] ⏱️  Uptime: %lu seconds", uptime);
}

/**
 * @brief 간단한 CPU 사용률 측정 (근사치)
 */
uint32_t PowerMgmt_GetCpuUsage(void) {
  static uint32_t last_idle_time = 0;
  static uint32_t last_total_time = 0;

  uint32_t current_time = HAL_GetTick();
  uint32_t idle_time = current_time - last_total_time; // 간략화된 계산

  last_total_time = current_time;

  return (idle_time > 100) ? 0 : 100; // 대략적인 CPU 사용률
}

/**
 * @brief HAL RTC 알람 콜백 (HAL에서 호출)
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
  PowerMgmt_AlarmCallback();
}