/**
 * @file system_config_runtime.h
 * @brief 런타임 시스템 설정 관리
 * @author Refactoring Phase 4
 * @date 2025-07-30
 * 
 * 시스템 전체 설정을 구조체 기반으로 관리하여 런타임에 변경 가능하도록 구현
 * 컴파일 타임 상수(#define)와 런타임 설정을 분리하여 유연성 극대화
 */

#ifndef SYSTEM_CONFIG_RUNTIME_H
#define SYSTEM_CONFIG_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include "error_codes.h"

// ============================================================================
// LoRa 통신 설정 구조체
// ============================================================================
typedef struct {
    uint32_t send_interval_ms;          // 메시지 전송 간격 (밀리초)
    uint32_t max_retry_count;           // 최대 재시도 횟수 (0 = 무제한)
    uint32_t retry_delay_ms;            // 재시도 간격 (밀리초)
    uint32_t time_sync_delay_ms;        // 시간 동기화 대기 시간
    uint32_t response_timeout_ms;       // 응답 대기 시간
    uint16_t message_number_max;        // 메시지 번호 최대값
    char default_message[32];           // 기본 전송 메시지
    bool auto_retry_enabled;            // 자동 재시도 활성화
    bool time_sync_enabled;             // 시간 동기화 활성화
} RuntimeLoRaConfig;

// ============================================================================
// UART 통신 설정 구조체
// ============================================================================
typedef struct {
    uint32_t baudrate;                  // 통신 속도
    uint16_t rx_buffer_size;            // 수신 버퍼 크기
    uint16_t tx_buffer_size;            // 송신 버퍼 크기
    uint32_t timeout_ms;                // 통신 타임아웃
    uint8_t data_bits;                  // 데이터 비트 (8)
    uint8_t stop_bits;                  // 스톱 비트 (1)
    uint8_t parity;                     // 패리티 (0=없음)
    bool flow_control_enabled;          // 플로우 컨트롤 활성화
} RuntimeUartConfig;

// ============================================================================
// SD 카드 설정 구조체
// ============================================================================
typedef struct {
    uint8_t mount_retry_count;          // 마운트 재시도 횟수
    uint32_t mount_retry_delay_ms;      // 마운트 재시도 간격
    uint32_t stabilize_delay_ms;        // 안정화 대기 시간
    uint8_t transfer_wait_max_count;    // TRANSFER 상태 대기 최대 횟수
    uint32_t transfer_check_interval_ms; // TRANSFER 상태 체크 간격
    uint32_t log_file_max_size;         // 로그 파일 최대 크기
    char log_directory[64];             // 로그 디렉토리 경로
    char log_file_prefix[32];           // 로그 파일 접두사
    bool auto_format_enabled;           // 자동 포맷 활성화
    bool compression_enabled;           // 압축 활성화
} RuntimeSDCardConfig;

// ============================================================================
// 로거 설정 구조체
// ============================================================================
typedef struct {
    uint16_t max_message_size;          // 최대 메시지 길이
    uint16_t write_buffer_size;         // 쓰기 버퍼 크기
    uint8_t sd_queue_size;              // SD 로그 큐 크기
    uint8_t log_level;                  // 기본 로그 레벨
    uint8_t sd_log_level;               // SD 로그 레벨
    bool timestamp_enabled;             // 타임스탬프 활성화
    bool dual_logging_enabled;          // 이중 로깅 (터미널+SD) 활성화
    bool async_logging_enabled;         // 비동기 로깅 활성화
} RuntimeLoggerConfig;

// ============================================================================
// 시스템 전반 설정 구조체
// ============================================================================
typedef struct {
    uint32_t boot_delay_ms;             // 부팅 지연 시간
    uint32_t stabilize_delay_ms;        // 일반 안정화 대기 시간
    uint16_t default_task_stack_size;   // 기본 태스크 스택 크기
    uint8_t system_priority;            // 시스템 우선순위
    bool debug_mode_enabled;            // 디버그 모드 활성화
    bool verbose_logging_enabled;       // 상세 로그 활성화
    bool test_mode_enabled;             // 테스트 모드 활성화
    bool watchdog_enabled;              // 워치독 활성화
} SystemConfig;

// ============================================================================
// 하드웨어 설정 구조체
// ============================================================================
typedef struct {
    uint8_t sdmmc_clock_div;            // SDMMC 클럭 분주비
    uint32_t dma_priority_level;        // DMA 우선순위
    uint32_t system_clock_mhz;          // 시스템 클럭 (MHz)
    bool dcache_enabled;                // D-Cache 활성화
    bool icache_enabled;                // I-Cache 활성화
} HardwareConfig;

// ============================================================================
// 통합 시스템 설정 구조체
// ============================================================================
typedef struct {
    // 개별 모듈 설정
    RuntimeLoRaConfig lora;             // LoRa 통신 설정
    RuntimeUartConfig uart;             // UART 통신 설정
    RuntimeSDCardConfig sd_card;        // SD 카드 설정
    RuntimeLoggerConfig logger;         // 로거 설정
    SystemConfig system;                // 시스템 전반 설정
    HardwareConfig hardware;            // 하드웨어 설정
    
    // 메타 정보
    uint32_t config_version;            // 설정 버전
    uint32_t config_crc;                // 설정 CRC 체크섬
    bool config_valid;                  // 설정 유효성
    char device_name[32];               // 장치 이름
    char firmware_version[16];          // 펌웨어 버전
} GlobalSystemConfig;

// ============================================================================
// 설정 관리 함수들
// ============================================================================

/**
 * @brief 시스템 설정 초기화 (기본값으로)
 * @param config 설정 구조체 포인터
 * @return 결과 코드
 */
ResultCode SystemConfig_Init(GlobalSystemConfig* config);

/**
 * @brief 시스템 설정 검증
 * @param config 설정 구조체 포인터
 * @return 결과 코드
 */
ResultCode SystemConfig_Validate(const GlobalSystemConfig* config);

/**
 * @brief 시스템 설정 적용
 * @param config 설정 구조체 포인터
 * @return 결과 코드
 */
ResultCode SystemConfig_Apply(const GlobalSystemConfig* config);

/**
 * @brief SD 카드에서 설정 로드
 * @param config 설정 구조체 포인터
 * @return 결과 코드
 */
ResultCode SystemConfig_LoadFromSD(GlobalSystemConfig* config);

/**
 * @brief SD 카드에 설정 저장
 * @param config 설정 구조체 포인터
 * @return 결과 코드
 */
ResultCode SystemConfig_SaveToSD(const GlobalSystemConfig* config);

/**
 * @brief 설정 팩토리 리셋
 * @param config 설정 구조체 포인터
 * @return 결과 코드
 */
ResultCode SystemConfig_FactoryReset(GlobalSystemConfig* config);

/**
 * @brief 글로벌 설정 인스턴스 가져오기
 * @return 글로벌 설정 포인터
 */
GlobalSystemConfig* SystemConfig_GetGlobal(void);

/**
 * @brief 특정 모듈 설정 가져오기
 */
RuntimeLoRaConfig* SystemConfig_GetLoRa(void);
RuntimeUartConfig* SystemConfig_GetUart(void);
RuntimeSDCardConfig* SystemConfig_GetSDCard(void);
RuntimeLoggerConfig* SystemConfig_GetLogger(void);
SystemConfig* SystemConfig_GetSystem(void);
HardwareConfig* SystemConfig_GetHardware(void);

// ============================================================================
// 설정 업데이트 매크로들 (타입 안전성 보장)
// ============================================================================

/**
 * @brief LoRa 설정 업데이트
 */
#define LORA_CONFIG_SET_INTERVAL(ms) \
    SystemConfig_GetLoRa()->send_interval_ms = (ms)

#define LORA_CONFIG_SET_RETRY_COUNT(count) \
    SystemConfig_GetLoRa()->max_retry_count = (count)

#define LORA_CONFIG_SET_MESSAGE(msg) \
    strncpy(SystemConfig_GetLoRa()->default_message, (msg), \
            sizeof(SystemConfig_GetLoRa()->default_message) - 1)

/**
 * @brief UART 설정 업데이트
 */
#define UART_CONFIG_SET_BAUDRATE(rate) \
    SystemConfig_GetUart()->baudrate = (rate)

#define UART_CONFIG_SET_BUFFER_SIZE(size) \
    SystemConfig_GetUart()->rx_buffer_size = (size)

/**
 * @brief SD 카드 설정 업데이트
 */
#define SD_CONFIG_SET_LOG_SIZE(size) \
    SystemConfig_GetSDCard()->log_file_max_size = (size)

#define SD_CONFIG_SET_DIRECTORY(dir) \
    strncpy(SystemConfig_GetSDCard()->log_directory, (dir), \
            sizeof(SystemConfig_GetSDCard()->log_directory) - 1)

/**
 * @brief 로거 설정 업데이트
 */
#define LOGGER_CONFIG_SET_LEVEL(level) \
    SystemConfig_GetLogger()->log_level = (level)

#define LOGGER_CONFIG_ENABLE_DUAL_LOGGING(enable) \
    SystemConfig_GetLogger()->dual_logging_enabled = (enable)

#endif // SYSTEM_CONFIG_RUNTIME_H