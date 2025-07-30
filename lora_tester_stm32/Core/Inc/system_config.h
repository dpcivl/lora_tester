/**
 * @file system_config.h
 * @brief 시스템 전체 설정값 중앙 관리
 * @date 2025-07-30
 */

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

// =============================================================================
// LoRa 통신 설정
// =============================================================================

/** LoRa 메시지 전송 간격 (밀리초) - 5분 */
#define LORA_SEND_INTERVAL_MS           300000

/** LoRa 재시도 최대 횟수 (0 = 무제한) */
#define LORA_MAX_RETRY_COUNT            0

/** LoRa 재시도 간격 (밀리초) */
#define LORA_RETRY_DELAY_MS             1000

/** 시간 동기화 대기 시간 (밀리초) */
#define LORA_TIME_SYNC_DELAY_MS         5000

/** LoRa 응답 대기 시간 (밀리초) */
#define LORA_RESPONSE_TIMEOUT_MS        3000

/** 메시지 번호 최대값 (0001~9999) */
#define LORA_MESSAGE_NUMBER_MAX         9999

// =============================================================================
// UART 통신 설정
// =============================================================================

/** UART 수신 버퍼 크기 */
#define UART_RX_BUFFER_SIZE             512

/** UART 통신 속도 */
#define UART_BAUDRATE                   115200

// =============================================================================
// SD 카드 설정
// =============================================================================

/** SD 카드 마운트 재시도 횟수 */
#define SD_MOUNT_RETRY_COUNT            3

/** SD 카드 마운트 재시도 간격 (밀리초) */
#define SD_MOUNT_RETRY_DELAY_MS         1000

/** SD 카드 안정화 대기 시간 (밀리초) */
#define SD_CARD_STABILIZE_DELAY_MS      500

/** SD 카드 TRANSFER 상태 대기 최대 시간 (100ms * 50 = 5초) */
#define SD_TRANSFER_WAIT_MAX_COUNT      50

/** SD 카드 TRANSFER 상태 체크 간격 (밀리초) */
#define SD_TRANSFER_CHECK_INTERVAL_MS   100

/** 로그 파일 최대 크기 (바이트) */
#define SD_LOG_FILE_MAX_SIZE            1000000

// =============================================================================
// 로거 설정
// =============================================================================

/** 로그 메시지 최대 길이 */
#define LOGGER_MAX_MESSAGE_SIZE         1024

/** 로그 쓰기 버퍼 크기 */
#define LOGGER_WRITE_BUFFER_SIZE        1024

/** SD 로그 큐 크기 */
#define LOGGER_SD_QUEUE_SIZE            10

// =============================================================================
// 시스템 설정
// =============================================================================

/** 시스템 부팅 지연 시간 (밀리초) */
#define SYSTEM_BOOT_DELAY_MS            5000

/** 일반적인 안정화 대기 시간 (밀리초) */
#define SYSTEM_STABILIZE_DELAY_MS       200

/** FreeRTOS 태스크 스택 크기 */
#define SYSTEM_DEFAULT_TASK_STACK_SIZE  4096

// =============================================================================
// 하드웨어 설정
// =============================================================================

/** SDMMC 클럭 분주비 */
#define SDMMC_CLOCK_DIV                 8

/** DMA 우선순위 */
#define DMA_PRIORITY_LEVEL              DMA_PRIORITY_HIGH

// =============================================================================
// 디버그 및 개발 설정
// =============================================================================

/** 디버그 모드 활성화 (0=비활성화, 1=활성화) */
#define DEBUG_MODE_ENABLED              1

/** 상세 로그 출력 활성화 */
#define VERBOSE_LOGGING_ENABLED         1

/** 테스트 모드 설정 */
#define TEST_MODE_ENABLED               0

#endif // SYSTEM_CONFIG_H