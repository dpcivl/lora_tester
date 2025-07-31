/**
 * @file system_config_runtime.c
 * @brief 런타임 시스템 설정 관리 구현
 * @author Refactoring Phase 4
 * @date 2025-07-30
 */

#include "system_config_runtime.h"
#include "system_config.h"
#include "logger.h"
#include <string.h>

// ============================================================================
// 전역 설정 인스턴스
// ============================================================================
static GlobalSystemConfig g_system_config = {0};
static bool g_config_initialized = false;

// ============================================================================
// 기본값 설정 함수들
// ============================================================================

/**
 * @brief LoRa 설정 기본값 설정
 */
static void _init_lora_config_defaults(RuntimeLoRaConfig* config)
{
    config->send_interval_ms = LORA_SEND_INTERVAL_MS;
    config->max_retry_count = LORA_MAX_RETRY_COUNT;
    config->retry_delay_ms = LORA_RETRY_DELAY_MS;
    config->time_sync_delay_ms = LORA_TIME_SYNC_DELAY_MS;
    config->response_timeout_ms = LORA_RESPONSE_TIMEOUT_MS;
    config->message_number_max = LORA_MESSAGE_NUMBER_MAX;
    strncpy(config->default_message, "TEST", sizeof(config->default_message) - 1);
    config->auto_retry_enabled = (LORA_MAX_RETRY_COUNT > 0);
    config->time_sync_enabled = true;
}

/**
 * @brief UART 설정 기본값 설정
 */
static void _init_uart_config_defaults(RuntimeUartConfig* config)
{
    config->baudrate = UART_BAUDRATE;
    config->rx_buffer_size = UART_RX_BUFFER_SIZE;
    config->tx_buffer_size = 256;  // 기본 송신 버퍼 크기
    config->timeout_ms = 1000;
    config->data_bits = 8;
    config->stop_bits = 1;
    config->parity = 0;  // 패리티 없음
    config->flow_control_enabled = false;
}

/**
 * @brief SD 카드 설정 기본값 설정
 */
static void _init_sd_config_defaults(RuntimeSDCardConfig* config)
{
    config->mount_retry_count = SD_MOUNT_RETRY_COUNT;
    config->mount_retry_delay_ms = SD_MOUNT_RETRY_DELAY_MS;
    config->stabilize_delay_ms = SD_CARD_STABILIZE_DELAY_MS;
    config->transfer_wait_max_count = SD_TRANSFER_WAIT_MAX_COUNT;
    config->transfer_check_interval_ms = SD_TRANSFER_CHECK_INTERVAL_MS;
    config->log_file_max_size = SD_LOG_FILE_MAX_SIZE;
    strncpy(config->log_directory, "lora_logs", sizeof(config->log_directory) - 1);
    strncpy(config->log_file_prefix, "LORA", sizeof(config->log_file_prefix) - 1);
    config->auto_format_enabled = true;
    config->compression_enabled = false;
}

/**
 * @brief 로거 설정 기본값 설정
 */
static void _init_logger_config_defaults(RuntimeLoggerConfig* config)
{
    config->max_message_size = LOGGER_MAX_MESSAGE_SIZE;
    config->write_buffer_size = LOGGER_WRITE_BUFFER_SIZE;
    config->sd_queue_size = LOGGER_SD_QUEUE_SIZE;
    config->log_level = LOG_LEVEL_INFO;
    config->sd_log_level = LOG_LEVEL_WARN;
    config->timestamp_enabled = true;
    config->dual_logging_enabled = true;
    config->async_logging_enabled = true;
}

/**
 * @brief 시스템 설정 기본값 설정
 */
static void _init_system_config_defaults(SystemConfig* config)
{
    config->boot_delay_ms = SYSTEM_BOOT_DELAY_MS;
    config->stabilize_delay_ms = SYSTEM_STABILIZE_DELAY_MS;
    config->default_task_stack_size = SYSTEM_DEFAULT_TASK_STACK_SIZE;
    config->system_priority = 2;  // 중간 우선순위
    config->debug_mode_enabled = DEBUG_MODE_ENABLED;
    config->verbose_logging_enabled = VERBOSE_LOGGING_ENABLED;
    config->test_mode_enabled = TEST_MODE_ENABLED;
    config->watchdog_enabled = false;  // 기본적으로 비활성화
}

/**
 * @brief 하드웨어 설정 기본값 설정
 */
static void _init_hardware_config_defaults(HardwareConfig* config)
{
    config->sdmmc_clock_div = SDMMC_CLOCK_DIV;
    config->dma_priority_level = DMA_PRIORITY_LEVEL;
    config->system_clock_mhz = 200;  // STM32F746 기본 클럭
    config->dcache_enabled = true;
    config->icache_enabled = true;
}

/**
 * @brief CRC 계산 (간단한 체크섬)
 */
static uint32_t _calculate_config_crc(const GlobalSystemConfig* config)
{
    const uint8_t* data = (const uint8_t*)config;
    uint32_t crc = 0;
    
    // CRC 필드를 제외한 전체 구조체에 대해 간단한 체크섬 계산
    size_t size = sizeof(GlobalSystemConfig) - sizeof(config->config_crc);
    
    for (size_t i = 0; i < size; i++) {
        crc += data[i];
        crc = (crc << 1) | (crc >> 31);  // 순환 시프트
    }
    
    return crc;
}

// ============================================================================
// 공개 함수 구현
// ============================================================================

/**
 * @brief 시스템 설정 초기화 (기본값으로)
 */
ResultCode SystemConfig_Init(GlobalSystemConfig* config)
{
    CHECK_NULL_PARAM(config);
    
    LOG_INFO("[SystemConfig] Initializing system configuration with defaults");
    
    // 전체 구조체 초기화
    memset(config, 0, sizeof(GlobalSystemConfig));
    
    // 각 모듈별 기본값 설정
    _init_lora_config_defaults(&config->lora);
    _init_uart_config_defaults(&config->uart);
    _init_sd_config_defaults(&config->sd_card);
    _init_logger_config_defaults(&config->logger);
    _init_system_config_defaults(&config->system);
    _init_hardware_config_defaults(&config->hardware);
    
    // 메타 정보 설정
    config->config_version = 1;
    strncpy(config->device_name, "LoRa-Tester-STM32", sizeof(config->device_name) - 1);
    strncpy(config->firmware_version, "v1.0.0", sizeof(config->firmware_version) - 1);
    
    // CRC 계산 및 유효성 설정
    config->config_crc = _calculate_config_crc(config);
    config->config_valid = true;
    
    LOG_INFO("[SystemConfig] Configuration initialized successfully (version: %lu)", config->config_version);
    return RESULT_SUCCESS;
}

/**
 * @brief 시스템 설정 검증
 */
ResultCode SystemConfig_Validate(const GlobalSystemConfig* config)
{
    CHECK_NULL_PARAM(config);
    
    LOG_DEBUG("[SystemConfig] Validating system configuration");
    
    // CRC 검증
    uint32_t calculated_crc = _calculate_config_crc(config);
    if (calculated_crc != config->config_crc) {
        LOG_ERROR("[SystemConfig] CRC mismatch: expected 0x%08lX, got 0x%08lX", 
                  config->config_crc, calculated_crc);
        return RESULT_ERROR_INVALID_PARAM;
    }
    
    // 기본 유효성 체크
    if (!config->config_valid) {
        LOG_ERROR("[SystemConfig] Configuration marked as invalid");
        return RESULT_ERROR_INVALID_PARAM;
    }
    
    // LoRa 설정 검증
    if (config->lora.send_interval_ms < 1000 || config->lora.send_interval_ms > 3600000) { // 1초~1시간
        LOG_ERROR("[SystemConfig] Invalid LoRa send interval: %lu ms", config->lora.send_interval_ms);
        return RESULT_ERROR_INVALID_PARAM;
    }
    
    // UART 설정 검증
    if (config->uart.baudrate < 9600 || config->uart.baudrate > 921600) {
        LOG_ERROR("[SystemConfig] Invalid UART baudrate: %lu", config->uart.baudrate);
        return RESULT_ERROR_INVALID_PARAM;
    }
    
    // SD 카드 설정 검증
    if (config->sd_card.log_file_max_size < 1024 || config->sd_card.log_file_max_size > 100*1024*1024) { // 1KB~100MB
        LOG_ERROR("[SystemConfig] Invalid SD log file max size: %lu bytes", config->sd_card.log_file_max_size);
        return RESULT_ERROR_INVALID_PARAM;
    }
    
    LOG_DEBUG("[SystemConfig] Configuration validation successful");
    return RESULT_SUCCESS;
}

/**
 * @brief 시스템 설정 적용
 */
ResultCode SystemConfig_Apply(const GlobalSystemConfig* config)
{
    CHECK_NULL_PARAM(config);
    
    // 설정 검증
    CHECK_RESULT(SystemConfig_Validate(config));
    
    LOG_INFO("[SystemConfig] Applying system configuration");
    
    // 글로벌 설정에 복사
    memcpy(&g_system_config, config, sizeof(GlobalSystemConfig));
    g_config_initialized = true;
    
    LOG_INFO("[SystemConfig] Configuration applied successfully");
    LOG_INFO("[SystemConfig] Device: %s, Firmware: %s", 
             config->device_name, config->firmware_version);
    LOG_INFO("[SystemConfig] LoRa interval: %lu ms, UART: %lu baud", 
             config->lora.send_interval_ms, config->uart.baudrate);
    
    return RESULT_SUCCESS;
}

/**
 * @brief SD 카드에서 설정 로드 (향후 구현)
 */
ResultCode SystemConfig_LoadFromSD(GlobalSystemConfig* config)
{
    CHECK_NULL_PARAM(config);
    
    LOG_INFO("[SystemConfig] Loading configuration from SD card (not implemented yet)");
    
    // 현재는 기본값으로 초기화
    return SystemConfig_Init(config);
}

/**
 * @brief SD 카드에 설정 저장 (향후 구현)
 */
ResultCode SystemConfig_SaveToSD(const GlobalSystemConfig* config)
{
    CHECK_NULL_PARAM(config);
    
    LOG_INFO("[SystemConfig] Saving configuration to SD card (not implemented yet)");
    
    // 향후 SDStorage를 사용하여 JSON 또는 바이너리 형태로 저장
    return RESULT_SUCCESS;
}

/**
 * @brief 설정 팩토리 리셋
 */
ResultCode SystemConfig_FactoryReset(GlobalSystemConfig* config)
{
    CHECK_NULL_PARAM(config);
    
    LOG_WARN("[SystemConfig] Performing factory reset of configuration");
    
    return SystemConfig_Init(config);
}

/**
 * @brief 글로벌 설정 인스턴스 가져오기
 */
GlobalSystemConfig* SystemConfig_GetGlobal(void)
{
    if (!g_config_initialized) {
        LOG_WARN("[SystemConfig] Configuration not initialized, using defaults");
        SystemConfig_Init(&g_system_config);
        SystemConfig_Apply(&g_system_config);
    }
    
    return &g_system_config;
}

/**
 * @brief 개별 모듈 설정 가져오기
 */
RuntimeLoRaConfig* SystemConfig_GetLoRa(void)
{
    return &SystemConfig_GetGlobal()->lora;
}

RuntimeUartConfig* SystemConfig_GetUart(void)
{
    return &SystemConfig_GetGlobal()->uart;
}

RuntimeSDCardConfig* SystemConfig_GetSDCard(void)
{
    return &SystemConfig_GetGlobal()->sd_card;
}

RuntimeLoggerConfig* SystemConfig_GetLogger(void)
{
    return &SystemConfig_GetGlobal()->logger;
}

SystemConfig* SystemConfig_GetSystem(void)
{
    return &SystemConfig_GetGlobal()->system;
}

HardwareConfig* SystemConfig_GetHardware(void)
{
    return &SystemConfig_GetGlobal()->hardware;
}

/**
 * @brief 시스템 설정 초기화 및 적용 래퍼 함수
 */
void SystemConfig_InitializeSystem(void)
{
    GlobalSystemConfig* config = SystemConfig_GetGlobal();
    if (SystemConfig_Init(config) == RESULT_SUCCESS) {
        SystemConfig_Apply(config);
        LOG_INFO("[SystemConfig] Runtime configuration system initialized");
    }
}