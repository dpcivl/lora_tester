#ifndef __POWER_MANAGEMENT_H__
#define __POWER_MANAGEMENT_H__

#include "main.h"

/* 저전력 모드 타입 */
typedef enum {
  POWER_MODE_RUN,    // 일반 동작 모드
  POWER_MODE_SLEEP,  // Sleep 모드 (CPU 중지, 페리페럴 동작)
  POWER_MODE_STOP,   // Stop 모드 (CPU + 페리페럴 중지)
  POWER_MODE_STANDBY // Standby 모드 (거의 모든 것 중지)
} PowerMode_t;

/* 저전력 모드 관리 함수들 */
void PowerMgmt_Init(void);
void PowerMgmt_EnterSleepMode(void);
void PowerMgmt_SetAlarmAndSleep(uint32_t seconds);
void PowerMgmt_WakeupFromAlarm(void);
uint8_t PowerMgmt_IsAlarmSet(void);

/* 전력 모니터링 함수들 */
void PowerMgmt_PrintSystemStatus(void);
uint32_t PowerMgmt_GetCpuUsage(void);

/* RTC 알람 콜백 */
void PowerMgmt_AlarmCallback(void);

#endif /* __POWER_MANAGEMENT_H__ */