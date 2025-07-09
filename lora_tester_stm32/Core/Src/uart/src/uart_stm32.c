/*
 * uart_stm32.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Lab2
 */

#include "uart.h"
#include "stm32f7xx_hal.h"
#include "logger.h"
#include <string.h>

extern UART_HandleTypeDef huart6; // CubeMX가 생성

// 전역 변수
static bool uart_initialized = false;
static uint8_t rx_buffer[256];  // 수신 버퍼
static volatile bool data_received = false;
static volatile int received_bytes = 0;

// 루프백 데이터 저장용 전역 버퍼
static uint8_t loopback_buffer[256];
static int loopback_count = 0;
static bool loopback_data_available = false;

// 수신 버퍼 플러시 함수
static void flush_rx_buffer(void) {
    uint8_t dummy;
    int flush_count = 0;
    
    LOG_INFO("[UART_STM32] Flushing RX buffer...");
    
    // 방법 1: 직접 레지스터 체크로 기존 데이터 클리어
    while (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_RXNE) && flush_count < 100) {
        dummy = (uint8_t)(huart6.Instance->RDR & 0xFF);
        flush_count++;
        LOG_DEBUG("[UART_STM32] Flushed byte %d: 0x%02X", flush_count, dummy);
    }
    
    // 방법 2: HAL로 남은 데이터 클리어 (타임아웃 1ms)
    while (HAL_UART_Receive(&huart6, &dummy, 1, 1) == HAL_OK && flush_count < 100) {
        flush_count++;
        LOG_DEBUG("[UART_STM32] HAL flushed byte %d: 0x%02X", flush_count, dummy);
    }
    
    if (flush_count > 0) {
        LOG_INFO("[UART_STM32] ✓ Flushed %d bytes from RX buffer", flush_count);
    } else {
        LOG_INFO("[UART_STM32] ✓ RX buffer was already empty");
    }
    
    // 루프백 버퍼도 초기화
    loopback_count = 0;
    loopback_data_available = false;
    memset(loopback_buffer, 0, sizeof(loopback_buffer));
}

UartStatus UART_Platform_Connect(const char* port) {
    // STM32에서는 이미 HAL_UART_Init()이 실행됨
    uart_initialized = true;
    data_received = false;
    received_bytes = 0;
    memset(rx_buffer, 0, sizeof(rx_buffer));
    
    // 루프백 버퍼 초기화
    loopback_count = 0;
    loopback_data_available = false;
    memset(loopback_buffer, 0, sizeof(loopback_buffer));
    
    // 초기 버퍼 플러시
    flush_rx_buffer();
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Disconnect(void) {
    uart_initialized = false;
    data_received = false;
    received_bytes = 0;
    
    // 루프백 버퍼 클리어
    loopback_count = 0;
    loopback_data_available = false;
    memset(loopback_buffer, 0, sizeof(loopback_buffer));
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Send(const char* data) {
    if (data == NULL || !uart_initialized) return UART_STATUS_ERROR;
    
    int len = strlen(data);
    if (len == 0) return UART_STATUS_OK;
    
    // 송신 전 수신 버퍼 플러시 (깨끗한 상태에서 시작)
    flush_rx_buffer();
    
    // 전송 데이터 로깅
    LOG_INFO("[UART_STM32] Sending %d bytes: '%s'", len, data);
    
    // 바이트별 hex 덤프
    LOG_INFO("[UART_STM32] Hex dump:");
    for (int i = 0; i < len; i++) {
        uint8_t byte = (uint8_t)data[i];
        char printable = (byte >= 32 && byte <= 126) ? byte : '.';
        LOG_INFO("  [%d] = 0x%02X ('%c') %s", i, byte, printable,
                (byte == 0x0D) ? "<CR>" : 
                (byte == 0x0A) ? "<LF>" : 
                (byte == 0x20) ? "<SPACE>" : "");
    }
    
    // 단순한 송신 (LoRa 모듈 전용)
    LOG_INFO("[UART_STM32] Starting LoRa module transmission...");
    
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart6, (uint8_t*)data, len, 1000);
    
    if (tx_status == HAL_OK) {
        LOG_INFO("[UART_STM32] ✓ Transmission completed successfully");
        
        // LoRa 모듈 응답 대기 시간 (더 충분한 시간)
        LOG_INFO("[UART_STM32] Waiting for LoRa module to process command...");
        HAL_Delay(200); // 200ms 대기 (LoRa 모듈 처리 시간)
        
        // 응답 데이터 수집 시작
        LOG_INFO("[UART_STM32] Starting response collection...");
        uint8_t response_buffer[256];
        int response_count = 0;
        
        // 적극적인 응답 수집 (10번 시도, 각 50ms 간격)
        for (int attempt = 1; attempt <= 10; attempt++) {
            LOG_DEBUG("[UART_STM32] Collection attempt %d/10", attempt);
            
            // 레지스터에서 가용한 모든 데이터 수집
            while (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_RXNE) && response_count < 255) {
                response_buffer[response_count] = (uint8_t)(huart6.Instance->RDR & 0xFF);
                LOG_DEBUG("[UART_STM32] Collected [%d]: 0x%02X ('%c')", 
                         response_count, response_buffer[response_count],
                         (response_buffer[response_count] >= 32 && response_buffer[response_count] <= 126) ? response_buffer[response_count] : '.');
                response_count++;
            }
            
            // HAL 함수로도 추가 수집
            uint8_t hal_byte;
            while (HAL_UART_Receive(&huart6, &hal_byte, 1, 5) == HAL_OK && response_count < 255) {
                response_buffer[response_count] = hal_byte;
                LOG_DEBUG("[UART_STM32] HAL collected [%d]: 0x%02X ('%c')", 
                         response_count, hal_byte,
                         (hal_byte >= 32 && hal_byte <= 126) ? hal_byte : '.');
                response_count++;
            }
            
            // Overrun 에러 체크 및 클리어
            if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_ORE)) {
                LOG_WARN("[UART_STM32] ⚠ Overrun error detected during collection - clearing");
                __HAL_UART_CLEAR_OREFLAG(&huart6);
            }
            
            // 데이터를 받았으면 조금 더 기다려서 연속 데이터 확인
            if (response_count > 0) {
                LOG_DEBUG("[UART_STM32] Got %d bytes, checking for more data...", response_count);
                HAL_Delay(20); // 20ms 더 대기
            } else {
                HAL_Delay(50); // 50ms 대기 후 재시도
            }
        }
        
        if (response_count > 0) {
            LOG_INFO("[UART_STM32] ✓ Collected %d response bytes from LoRa module", response_count);
            
            // 루프백 버퍼에 저장
            if (response_count < sizeof(loopback_buffer)) {
                memcpy(loopback_buffer, response_buffer, response_count);
                loopback_count = response_count;
                loopback_data_available = true;
                LOG_INFO("[UART_STM32] ✓ Response data saved to buffer");
            } else {
                LOG_WARN("[UART_STM32] ⚠ Response data too large, truncating");
                memcpy(loopback_buffer, response_buffer, sizeof(loopback_buffer));
                loopback_count = sizeof(loopback_buffer);
                loopback_data_available = true;
            }
            
            // 수집된 응답 데이터 로깅
            LOG_INFO("[UART_STM32] LoRa response preview:");
            for (int i = 0; i < response_count; i++) {
                char printable = (response_buffer[i] >= 32 && response_buffer[i] <= 126) ? response_buffer[i] : '.';
                const char* special = "";
                if (response_buffer[i] == 0x0D) special = " <CR>";
                else if (response_buffer[i] == 0x0A) special = " <LF>";
                else if (response_buffer[i] == 0x20) special = " <SPACE>";
                
                LOG_INFO("[UART_STM32]   [%d] = 0x%02X ('%c')%s", i, response_buffer[i], printable, special);
            }
        } else {
            LOG_WARN("[UART_STM32] ⚠ No response data collected from LoRa module");
        }
        
        return UART_STATUS_OK;
    } else {
        LOG_ERROR("[UART_STM32] ✗ Transmission failed (HAL status: %d)", tx_status);
        return UART_STATUS_ERROR;
    }
}

UartStatus UART_Platform_Receive(char* buffer, int buffer_size, int* bytes_received) {
    if (buffer == NULL || bytes_received == NULL || !uart_initialized) {
        return UART_STATUS_ERROR;
    }
    
    if (buffer_size <= 0) {
        *bytes_received = 0;
        return UART_STATUS_ERROR;
    }
    
    *bytes_received = 0;
    
    LOG_INFO("[UART_STM32] === Starting IMPROVED receive ===");
    
    // 1단계: 루프백 버퍼에 데이터가 있는지 확인
    if (loopback_data_available) {
        LOG_INFO("[UART_STM32] ✓ Using loopback buffer data (%d bytes)", loopback_count);
        
        // 루프백 버퍼에서 데이터 복사
        int copy_count = (loopback_count < buffer_size - 1) ? loopback_count : buffer_size - 1;
        memcpy(buffer, loopback_buffer, copy_count);
        *bytes_received = copy_count;
        buffer[*bytes_received] = '\0';
        
        // 루프백 데이터 로깅
        LOG_INFO("[UART_STM32] Loopback data returned:");
        for (int i = 0; i < *bytes_received; i++) {
            uint8_t byte = (uint8_t)buffer[i];
            char printable = (byte >= 32 && byte <= 126) ? byte : '.';
            const char* special = "";
            if (byte == 0x0D) special = " <CR>";
            else if (byte == 0x0A) special = " <LF>";
            else if (byte == 0x20) special = " <SPACE>";
            
            LOG_INFO("[UART_STM32]   [%d] = 0x%02X ('%c')%s", i, byte, printable, special);
        }
        
        // 루프백 버퍼 클리어
        loopback_data_available = false;
        loopback_count = 0;
        
        LOG_INFO("[UART_STM32] === IMPROVED receive COMPLETE (from loopback) ===");
        return UART_STATUS_OK;
    }
    
    // 2단계: 실시간 수신 로직
    LOG_INFO("[UART_STM32] No loopback data, checking real-time reception");
    
    // 수신 시작 전 상태 확인
    LOG_INFO("[UART_STM32] Pre-receive status:");
    bool rxne_flag = __HAL_UART_GET_FLAG(&huart6, UART_FLAG_RXNE);
    bool ore_flag = __HAL_UART_GET_FLAG(&huart6, UART_FLAG_ORE);
    LOG_INFO("[UART_STM32]   RXNE flag: %s", rxne_flag ? "SET" : "CLEAR");
    LOG_INFO("[UART_STM32]   ORE flag: %s", ore_flag ? "SET" : "CLEAR");
    
    if (ore_flag) {
        LOG_WARN("[UART_STM32] ⚠ Overrun error detected - clearing");
        __HAL_UART_CLEAR_OREFLAG(&huart6);
    }
    
    // 개선된 수신 로직: 먼저 모든 가용 데이터를 빠르게 수집
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout_total = 2000; // 2초 전체 타임아웃
    uint32_t inter_byte_timeout = 100; // 바이트 간 타임아웃 100ms로 단축
    uint32_t last_byte_time = start_time;
    
    LOG_INFO("[UART_STM32] Starting data collection with timeouts:");
    LOG_INFO("[UART_STM32]   Total timeout: %lu ms", timeout_total);
    LOG_INFO("[UART_STM32]   Inter-byte timeout: %lu ms", inter_byte_timeout);
    
    while (*bytes_received < buffer_size - 1) {
        uint32_t current_time = HAL_GetTick();
        uint32_t elapsed_total = current_time - start_time;
        uint32_t elapsed_since_last = current_time - last_byte_time;
        
        // 전체 타임아웃 체크
        if (elapsed_total > timeout_total) {
            LOG_INFO("[UART_STM32] Total timeout (%lu ms) reached", elapsed_total);
            break;
        }
        
        // 바이트 간 타임아웃 체크 (첫 번째 바이트가 없으면 더 오래 기다림)
        uint32_t current_inter_byte_timeout = (*bytes_received == 0) ? 1000 : inter_byte_timeout;
        if (elapsed_since_last > current_inter_byte_timeout) {
            LOG_INFO("[UART_STM32] Inter-byte timeout (%lu ms) after %d bytes", 
                    current_inter_byte_timeout, *bytes_received);
            break;
        }
        
        bool data_received_this_loop = false;
        
        // 방법 1: 직접 레지스터 체크 (가장 빠름)
        if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_RXNE)) {
            uint8_t received_byte = (uint8_t)(huart6.Instance->RDR & 0xFF);
            buffer[*bytes_received] = received_byte;
            (*bytes_received)++;
            data_received_this_loop = true;
            last_byte_time = current_time;
            
            char printable = (received_byte >= 32 && received_byte <= 126) ? received_byte : '.';
            const char* special = "";
            if (received_byte == 0x0D) special = " <CR>";
            else if (received_byte == 0x0A) special = " <LF>";
            else if (received_byte == 0x20) special = " <SPACE>";
            
            LOG_INFO("[UART_STM32] ✓ Register read [%d]: 0x%02X ('%c')%s (elapsed: %lu ms)", 
                    *bytes_received - 1, received_byte, printable, special, elapsed_total);
            
            // 완전한 응답 확인 (LF 또는 특정 패턴)
            if (received_byte == '\n') {
                LOG_INFO("[UART_STM32] Found LF - complete response received");
                break;
            }
        }
        
        // 방법 2: HAL 함수 시도 (레지스터로 안 받았을 때만)
        if (!data_received_this_loop) {
            uint8_t single_byte;
            HAL_StatusTypeDef hal_status = HAL_UART_Receive(&huart6, &single_byte, 1, 1);
            
            if (hal_status == HAL_OK) {
                buffer[*bytes_received] = single_byte;
                (*bytes_received)++;
                data_received_this_loop = true;
                last_byte_time = current_time;
                
                char printable = (single_byte >= 32 && single_byte <= 126) ? single_byte : '.';
                const char* special = "";
                if (single_byte == 0x0D) special = " <CR>";
                else if (single_byte == 0x0A) special = " <LF>";
                else if (single_byte == 0x20) special = " <SPACE>";
                
                LOG_INFO("[UART_STM32] ✓ HAL read [%d]: 0x%02X ('%c')%s (elapsed: %lu ms)", 
                        *bytes_received - 1, single_byte, printable, special, elapsed_total);
                
                if (single_byte == '\n') {
                    LOG_INFO("[UART_STM32] Found LF - complete response received");
                    break;
                }
            }
        }
        
        // 데이터를 못 받았으면 짧은 지연
        if (!data_received_this_loop) {
            HAL_Delay(1); // 1ms 지연
        }
    }
    
    // 버퍼 null-terminate
    buffer[*bytes_received] = '\0';
    
    LOG_INFO("[UART_STM32] === IMPROVED receive COMPLETE (real-time) ===");
    LOG_INFO("[UART_STM32] Total bytes received: %d", *bytes_received);
    LOG_INFO("[UART_STM32] String format: '%s'", buffer);
    LOG_INFO("[UART_STM32] Raw hex analysis:");
    for (int i = 0; i < *bytes_received; i++) {
        uint8_t byte = (uint8_t)buffer[i];
        char printable = (byte >= 32 && byte <= 126) ? byte : '.';
        const char* special = "";
        if (byte == 0x0D) special = " <CR>";
        else if (byte == 0x0A) special = " <LF>";
        else if (byte == 0x20) special = " <SPACE>";
        
        LOG_INFO("[UART_STM32]   [%d] = 0x%02X ('%c')%s", i, byte, printable, special);
    }
    
    return (*bytes_received > 0) ? UART_STATUS_OK : UART_STATUS_TIMEOUT;
}

UartStatus UART_Platform_Configure(const UartConfig* config) {
    if (config == NULL) return UART_STATUS_ERROR;
    
    // 새로운 설정 적용
    huart6.Init.BaudRate = config->baud_rate;
    huart6.Init.WordLength = (config->data_bits == 8) ? UART_WORDLENGTH_8B : UART_WORDLENGTH_9B;
    huart6.Init.StopBits = (config->stop_bits == 1) ? UART_STOPBITS_1 : UART_STOPBITS_2;
    huart6.Init.Parity = (config->parity == 0) ? UART_PARITY_NONE : 
                            (config->parity == 1) ? UART_PARITY_ODD : UART_PARITY_EVEN;
    
    // UART 재초기화
    if (HAL_UART_DeInit(&huart6) == HAL_OK && 
        HAL_UART_Init(&huart6) == HAL_OK) {
        return UART_STATUS_OK;
    }
    
    return UART_STATUS_ERROR;
}
