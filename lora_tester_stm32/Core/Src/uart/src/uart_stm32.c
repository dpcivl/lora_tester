/*
 * uart_stm32.c
 *
 *  Created on: Jun 24, 2025
 *      Author: Lab2
 */

#include "uart.h"
#include "stm32f7xx_hal.h"
#include "logger.h"
#include "cmsis_os.h"
#include <string.h>

extern UART_HandleTypeDef huart6; // CubeMX가 생성

// DMA 관련 변수들 (main.c에서도 사용하므로 extern으로 노출)
volatile uint8_t uart_rx_complete_flag = 0;
volatile uint8_t uart_rx_error_flag = 0;
volatile uint16_t uart_rx_length = 0;
char rx_buffer[512];  // 512바이트로 확대

// 내부 상태 변수들
static bool uart_initialized = false;
static bool dma_receiving = false;

// 수신 버퍼 플러시 함수
static void flush_rx_buffer(void) {
    uint8_t dummy;
    int flush_count = 0;
    
    // 방법 1: 직접 레지스터 체크로 기존 데이터 클리어
    while (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_RXNE) && flush_count < 100) {
        dummy = (uint8_t)(huart6.Instance->RDR & 0xFF);
        flush_count++;
    }
    
    // 방법 2: HAL로 남은 데이터 클리어 (타임아웃 1ms)
    while (HAL_UART_Receive(&huart6, &dummy, 1, 1) == HAL_OK && flush_count < 100) {
        flush_count++;
    }
}

UartStatus UART_Platform_Connect(const char* port) {
    // STM32에서는 이미 HAL_UART_Init()이 실행됨
    uart_initialized = true;
    
    // UART 상태 체크 및 리셋
    LOG_INFO("[UART_STM32] UART gState: %d, RxState: %d", 
             huart6.gState, huart6.RxState);
    
    // DMA 핸들 연결 상태 확인
    if (huart6.hdmarx != NULL) {
        LOG_INFO("[UART_STM32] DMA RX handle is connected");
        LOG_INFO("[UART_STM32] DMA State: %d", huart6.hdmarx->State);
    } else {
        LOG_ERROR("[UART_STM32] DMA RX handle is NULL - DMA not initialized!");
        
        // DMA 핸들 강제 연결 시도
        extern DMA_HandleTypeDef hdma_usart6_rx;
        __HAL_LINKDMA(&huart6, hdmarx, hdma_usart6_rx);
        
        if (huart6.hdmarx != NULL) {
            LOG_INFO("[UART_STM32] DMA RX handle manually linked");
        } else {
            LOG_ERROR("[UART_STM32] Failed to link DMA RX handle");
            return UART_STATUS_ERROR;
        }
    }
    
    // 이전에 시작된 DMA 작업이 있으면 중지
    if (dma_receiving) {
        HAL_UART_DMAStop(&huart6);
        dma_receiving = false;
        LOG_INFO("[UART_STM32] Previous DMA reception stopped");
    }
    
    // UART 상태를 READY로 강제 설정
    huart6.gState = HAL_UART_STATE_READY;
    huart6.RxState = HAL_UART_STATE_READY;
    
    // DMA 상태도 READY로 설정
    if (huart6.hdmarx != NULL) {
        // DMA 재초기화 (기존 상태 문제 해결)
        if (huart6.hdmarx->State != HAL_DMA_STATE_READY) {
            LOG_INFO("[UART_STM32] DMA not ready, reinitializing...");
            HAL_DMA_DeInit(huart6.hdmarx);
            if (HAL_DMA_Init(huart6.hdmarx) != HAL_OK) {
                LOG_ERROR("[UART_STM32] DMA reinitialization failed");
                return UART_STATUS_ERROR;
            }
            LOG_INFO("[UART_STM32] DMA reinitialized successfully");
        }
        huart6.hdmarx->State = HAL_DMA_STATE_READY;
    }
    
    // 초기 버퍼 플러시
    flush_rx_buffer();
    
    // DMA 기반 연속 수신 시작
    uart_rx_complete_flag = 0;
    uart_rx_error_flag = 0;
    uart_rx_length = 0;
    
    // DMA 수신 버퍼 클리어
    memset(rx_buffer, 0, sizeof(rx_buffer));
    
    LOG_INFO("[UART_STM32] Starting DMA reception...");
    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(&huart6, (uint8_t*)rx_buffer, sizeof(rx_buffer));
    if (status == HAL_OK) {
        dma_receiving = true;
        LOG_INFO("[UART_STM32] ✓ DMA continuous reception started (buffer size: %d)", sizeof(rx_buffer));
    } else {
        LOG_ERROR("[UART_STM32] ✗ Failed to start DMA reception (status: %d)", status);
        LOG_ERROR("[UART_STM32] UART gState after failure: %d, RxState: %d", 
                  huart6.gState, huart6.RxState);
        return UART_STATUS_ERROR;
    }
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Disconnect(void) {
    // DMA 수신 중지
    if (dma_receiving) {
        HAL_UART_DMAStop(&huart6);
        dma_receiving = false;
        LOG_INFO("[UART_STM32] ✓ DMA reception stopped");
    }
    
    uart_initialized = false;
    
    return UART_STATUS_OK;
}

UartStatus UART_Platform_Send(const char* data) {
    if (data == NULL || !uart_initialized) return UART_STATUS_ERROR;
    
    int len = strlen(data);
    if (len == 0) return UART_STATUS_OK;
    
    // 송신 전 수신 버퍼 플러시 (깨끗한 상태에서 시작)
    flush_rx_buffer();
    
    // 단순한 송신
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart6, (uint8_t*)data, len, 1000);
    
    if (tx_status == HAL_OK) {
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
    
    // DMA 수신이 시작되지 않았으면 에러
    if (!dma_receiving) {
        return UART_STATUS_ERROR;
    }
    
    // 에러 체크
    if (uart_rx_error_flag) {
        uart_rx_error_flag = 0;  // 플래그 클리어
        LOG_WARN("[UART_STM32] ⚠ DMA reception error occurred");
        return UART_STATUS_ERROR;
    }
    
    // DMA 수신 완료 체크
    if (uart_rx_complete_flag) {
        uart_rx_complete_flag = 0;  // 플래그 클리어
        
        // 실제 수신된 바이트 수 확인
        uint16_t received_length = uart_rx_length;
        LOG_INFO("[UART_STM32] DMA received %d bytes", received_length);
        
        if (received_length > 0 && received_length <= buffer_size - 1) {
            // 데이터 복사
            memcpy(buffer, rx_buffer, received_length);
            buffer[received_length] = '\0';  // null terminate
            *bytes_received = received_length;
            
            // 수신된 데이터 로그 (간단하게)
            LOG_INFO("[UART_STM32] Received data (%d bytes): '%s'", received_length, buffer);
            
            // 새로운 수신을 위해 DMA 완전 리셋 후 재시작
            memset(rx_buffer, 0, sizeof(rx_buffer));
            
            // 1. DMA 완전 정지
            HAL_UART_DMAStop(&huart6);
            
            // 2. 모든 UART 에러 플래그 클리어
            __HAL_UART_CLEAR_FLAG(&huart6, UART_CLEAR_PEF);
            __HAL_UART_CLEAR_FLAG(&huart6, UART_CLEAR_FEF);
            __HAL_UART_CLEAR_FLAG(&huart6, UART_CLEAR_NEF);
            __HAL_UART_CLEAR_FLAG(&huart6, UART_CLEAR_OREF);
            __HAL_UART_CLEAR_FLAG(&huart6, UART_CLEAR_IDLEF);
            
            // 3. DMA 스트림이 완전히 정지될 때까지 대기
            if (huart6.hdmarx != NULL) {
                int timeout = 1000;
                while (huart6.hdmarx->State != HAL_DMA_STATE_READY && timeout > 0) {
                    timeout--;
                    for(volatile int i = 0; i < 100; i++); // 짧은 지연
                }
                
                if (timeout == 0) {
                    LOG_WARN("[UART_STM32] DMA did not reach READY state, forcing reset");
                    huart6.hdmarx->State = HAL_DMA_STATE_READY;
                }
            }
            
            // 4. UART 상태 리셋 (DMA 완전 정지 후)
            huart6.RxState = HAL_UART_STATE_READY;
            huart6.gState = HAL_UART_STATE_READY;
            
            // 5. 충분한 지연 후 재시작
            for(volatile int i = 0; i < 10000; i++); // 더 긴 지연
            
            // 6. DMA 재시작
            HAL_StatusTypeDef restart_status = HAL_UART_Receive_DMA(&huart6, (uint8_t*)rx_buffer, sizeof(rx_buffer));
            if (restart_status == HAL_OK) {
                LOG_DEBUG("[UART_STM32] DMA restarted for next reception");
            } else {
                LOG_WARN("[UART_STM32] DMA restart failed (status: %d), UART state: g=%d rx=%d", 
                        restart_status, huart6.gState, huart6.RxState);
                if (huart6.hdmarx != NULL) {
                    LOG_WARN("[UART_STM32] DMA state: %d", huart6.hdmarx->State);
                }
            }
            
            return UART_STATUS_OK;
        } else {
            LOG_WARN("[UART_STM32] Invalid received length: %d (buffer size: %d)", received_length, buffer_size);
        }
    }
    
    // 수신된 데이터 없음
    return UART_STATUS_TIMEOUT;
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

// ============================================================================
// HAL UART 콜백 함수들 - main.c에서 이동됨
// ============================================================================

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    // DMA 수신 완료 (전체 버퍼) - 거의 발생하지 않음
    uart_rx_complete_flag = 1;
    uart_rx_length = sizeof(rx_buffer);
    LOG_INFO("[DMA] RxCpltCallback: Full buffer received (%d bytes)", uart_rx_length);
  }
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    // DMA 수신 절반 완료 - NORMAL 모드에서는 처리하지 않음 (IDLE 인터럽트가 처리)
    LOG_WARN("[DMA] RxHalfCpltCallback: Half buffer reached but ignoring in NORMAL mode");
    // uart_rx_complete_flag는 설정하지 않음 - IDLE 인터럽트에서만 설정
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    // UART 에러 발생
    uart_rx_error_flag = 1;
    LOG_WARN("[DMA] ErrorCallback: UART error occurred");
    
    // 모든 에러 플래그 클리어
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) {
      __HAL_UART_CLEAR_OREFLAG(huart);
      LOG_WARN("[DMA] Overrun error cleared");
    }
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_NE)) {
      __HAL_UART_CLEAR_NEFLAG(huart);
      LOG_WARN("[DMA] Noise error cleared");
    }
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_FE)) {
      __HAL_UART_CLEAR_FEFLAG(huart);
      LOG_WARN("[DMA] Frame error cleared");
    }
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_PE)) {
      __HAL_UART_CLEAR_PEFLAG(huart);
      LOG_WARN("[DMA] Parity error cleared");
    }
    
    // UART와 DMA 상태 강제 리셋
    HAL_UART_DMAStop(huart);
    huart->gState = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;
    if (huart->hdmarx != NULL) {
      huart->hdmarx->State = HAL_DMA_STATE_READY;
    }
    
    // 버퍼 클리어 후 DMA 재시작 (일반 모드)
    memset(rx_buffer, 0, sizeof(rx_buffer));
    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(huart, (uint8_t*)rx_buffer, sizeof(rx_buffer));
    if (status == HAL_OK) {
      LOG_INFO("[DMA] Error recovery: DMA restarted successfully");
    } else {
      LOG_ERROR("[DMA] Error recovery: DMA restart failed (status: %d)", status);
    }
  }
}

// UART IDLE 인터럽트 콜백 (메시지 끝 감지)
void USER_UART_IDLECallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    // UART 에러 상태 체크
    uint32_t error_flags = 0;
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) error_flags |= 0x01;
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_FE)) error_flags |= 0x02;
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_NE)) error_flags |= 0x04;
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_PE)) error_flags |= 0x08;
    
    // IDLE 감지 - 메시지 끝
    uint16_t remaining = __HAL_DMA_GET_COUNTER(huart->hdmarx);
    uart_rx_length = sizeof(rx_buffer) - remaining;
    
    if (uart_rx_length > 0) {
      uart_rx_complete_flag = 1;
      if (error_flags != 0) {
        LOG_WARN("[DMA] IDLE detected: %d bytes received (UART errors: 0x%02lX)", uart_rx_length, error_flags);
      } else {
        LOG_INFO("[DMA] IDLE detected: %d bytes received", uart_rx_length);
      }
      
      // 첫 몇 바이트 확인 (디버깅용)
      if (uart_rx_length >= 4) {
        LOG_DEBUG("[DMA] First 4 bytes: %02X %02X %02X %02X", 
                  rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
      }
      
      // DMA 중지 (일반 모드에서는 자동으로 완료됨)
      HAL_UART_DMAStop(huart);
      
      // 다음 수신을 위해 즉시 재시작하지 않음 - uart_stm32.c에서 처리
    } else {
      LOG_DEBUG("[DMA] IDLE detected but no data");
    }
  }
}

