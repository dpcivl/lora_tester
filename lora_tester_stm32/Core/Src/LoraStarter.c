#include "LoraStarter.h"
#include "uart.h"
#include "CommandSender.h"
#include "ResponseHandler.h"
#include "time.h"
#include "logger.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// LoRa 기본 초기화 명령어 배열 (TDD에서 검증된 명령어들)
const char* LORA_DEFAULT_INIT_COMMANDS[] = {
    "AT\r\n",           // 버전 확인 (연결 테스트)
    "AT+NWM=1\r\n",     // LoRaWAN 모드 설정
    "AT+NJM=1\r\n",     // OTAA 모드 설정
    "AT+CLASS=A\r\n",   // Class A 설정
    "AT+BAND=7\r\n"     // Asia 923 MHz 대역 설정
    // AT+TIMEREQ=1은 JOIN 후에 별도로 실행
};

const int LORA_DEFAULT_INIT_COMMANDS_COUNT = sizeof(LORA_DEFAULT_INIT_COMMANDS) / sizeof(LORA_DEFAULT_INIT_COMMANDS[0]);

// 상태 이름을 문자열로 변환하는 헬퍼 함수
static const char* get_state_name(LoraState state) {
    switch(state) {
        case LORA_STATE_INIT: return "INIT";
        case LORA_STATE_SEND_CMD: return "SEND_CMD";
        case LORA_STATE_WAIT_OK: return "WAIT_OK";
        case LORA_STATE_SEND_JOIN: return "SEND_JOIN";
        case LORA_STATE_WAIT_JOIN_OK: return "WAIT_JOIN_OK";
        case LORA_STATE_SEND_TIMEREQ: return "SEND_TIMEREQ";
        case LORA_STATE_WAIT_TIMEREQ_OK: return "WAIT_TIMEREQ_OK";
        case LORA_STATE_WAIT_TIME_SYNC: return "WAIT_TIME_SYNC";
        case LORA_STATE_SEND_LTIME: return "SEND_LTIME";
        case LORA_STATE_WAIT_LTIME_RESPONSE: return "WAIT_LTIME_RESPONSE";
        case LORA_STATE_SEND_PERIODIC: return "SEND_PERIODIC";
        case LORA_STATE_WAIT_SEND_RESPONSE: return "WAIT_SEND_RESPONSE";
        case LORA_STATE_WAIT_SEND_INTERVAL: return "WAIT_SEND_INTERVAL";
        case LORA_STATE_JOIN_RETRY: return "JOIN_RETRY";
        case LORA_STATE_DONE: return "DONE";
        case LORA_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void LoraStarter_ConnectUART(const char* port)
{
    UART_Connect(port);
    LOG_INFO("[LoRa] UART connected to %s", port);
}

void LoraStarter_InitWithDefaults(LoraStarterContext* ctx, const char* send_message)
{
    if (ctx == NULL) return;
    
    ctx->state = LORA_STATE_INIT;
    ctx->cmd_index = 0;
    ctx->commands = LORA_DEFAULT_INIT_COMMANDS;
    ctx->num_commands = LORA_DEFAULT_INIT_COMMANDS_COUNT;
    ctx->send_message = (send_message != NULL) ? send_message : "TEST";
    ctx->max_retry_count = 3;
    ctx->send_interval_ms = 300000;  // 5분 간격
    ctx->last_send_time = 0;
    ctx->send_count = 0;
    ctx->error_count = 0;
    ctx->last_retry_time = 0;
    ctx->retry_delay_ms = 1000;  // 1초 초기 지연
    
    LOG_INFO("[LoRa] Initialized with defaults - Commands: %d, Message: %s", 
             ctx->num_commands, ctx->send_message);
}

void LoraStarter_Process(LoraStarterContext* ctx, const char* uart_rx)
{
    if (ctx == NULL) return;

    LoraState old_state = ctx->state;

    switch(ctx->state) {
        case LORA_STATE_INIT:
            ctx->cmd_index = 0;
            ctx->error_count = 0;
            ctx->state = LORA_STATE_SEND_CMD;
            // 기본값 설정
            if (ctx->max_retry_count == 0) ctx->max_retry_count = 0; // 0이면 무제한
            if (ctx->send_message == NULL) ctx->send_message = "Hello";
            ctx->last_retry_time = 0;
            ctx->retry_delay_ms = 1000; // 초기 재시도 지연: 1초
            LOG_INFO("[LoRa] Initialized with message: %s, max_retries: %d", 
                    ctx->send_message, ctx->max_retry_count);
            break;
        case LORA_STATE_SEND_CMD:
            if (ctx->cmd_index < ctx->num_commands) {
                LOG_DEBUG("[LoRa] Sending command %d/%d: %s", 
                         ctx->cmd_index + 1, ctx->num_commands, ctx->commands[ctx->cmd_index]);
                CommandSender_Send(ctx->commands[ctx->cmd_index]);
                ctx->state = LORA_STATE_WAIT_OK;
            } else {
                ctx->state = LORA_STATE_SEND_JOIN;
            }
            break;
        case LORA_STATE_WAIT_OK:
            if (uart_rx) {
                if (is_response_ok(uart_rx)) {
                    LOG_DEBUG("[LoRa] Command %d OK received", ctx->cmd_index + 1);
                    ctx->cmd_index++;
                    ctx->state = LORA_STATE_SEND_CMD;
                } else if (strstr(uart_rx, "ERROR") || strstr(uart_rx, "AT_COMMAND_NOT_FOUND")) {
                    LOG_WARN("[LoRa] Command %d failed: %s", ctx->cmd_index + 1, uart_rx);
                    ctx->error_count++;
                    
                    // 에러 처리: 최대 3번 재시도 후 다음 명령으로 건너뛰기
                    if (ctx->error_count < 3) {
                        LOG_INFO("[LoRa] Retrying command %d (attempt %d/3)", ctx->cmd_index + 1, ctx->error_count + 1);
                        ctx->state = LORA_STATE_SEND_CMD; // 같은 명령 재시도
                    } else {
                        LOG_WARN("[LoRa] Command %d failed after 3 attempts, skipping to next", ctx->cmd_index + 1);
                        ctx->cmd_index++; // 다음 명령으로 건너뛰기
                        ctx->error_count = 0; // 에러 카운터 리셋
                        ctx->state = LORA_STATE_SEND_CMD;
                    }
                }
            }
            break;
        case LORA_STATE_SEND_JOIN:
            LORA_LOG_JOIN_ATTEMPT();
            CommandSender_Send("AT+JOIN\r\n");
            ctx->state = LORA_STATE_WAIT_JOIN_OK;
            break;
        case LORA_STATE_WAIT_JOIN_OK:
            if (uart_rx && is_join_response_ok(uart_rx)) {
                LORA_LOG_JOIN_SUCCESS();
                ctx->state = LORA_STATE_SEND_TIMEREQ; // JOIN 후 시간 동기화 활성화로 전환
                ctx->send_count = 0;
                ctx->error_count = 0; // JOIN 성공 시 에러 카운터 리셋
                ctx->retry_delay_ms = 1000; // 재시도 지연 시간 리셋
                ctx->last_retry_time = 0; // 재시도 시간 리셋
                LOG_INFO("[LoRa] JOIN successful, requesting time synchronization...");
            }
            break;
        case LORA_STATE_SEND_TIMEREQ:
            LOG_INFO("[LoRa] Sending time synchronization request...");
            CommandSender_Send("AT+TIMEREQ=1\r\n");
            ctx->state = LORA_STATE_WAIT_TIMEREQ_OK;
            break;
        case LORA_STATE_WAIT_TIMEREQ_OK:
            if (uart_rx && is_response_ok(uart_rx)) {
                LOG_WARN("[LoRa] ✅ Time synchronization enabled");
                ctx->state = LORA_STATE_WAIT_TIME_SYNC;
                ctx->last_retry_time = TIME_GetCurrentMs(); // 5초 지연 시작 시점 기록
            }
            break;
        case LORA_STATE_WAIT_TIME_SYNC:
            {
                uint32_t current_time = TIME_GetCurrentMs();
                const uint32_t TIME_SYNC_DELAY_MS = 5000; // 5초 대기
                
                if (ctx->last_retry_time == 0) {
                    // 처음 진입 시 시작 시간 기록
                    ctx->last_retry_time = current_time;
                    LOG_INFO("[LoRa] ⏳ Waiting 5 seconds for time synchronization...");
                } else if ((current_time - ctx->last_retry_time) >= TIME_SYNC_DELAY_MS) {
                    // 5초 경과 시 LTIME 명령 실행
                    LOG_INFO("[LoRa] ✅ Time sync delay completed, requesting network time");
                    ctx->state = LORA_STATE_SEND_LTIME;
                    ctx->last_retry_time = 0; // 타이머 리셋
                }
            }
            break;
        case LORA_STATE_SEND_LTIME:
            LOG_INFO("[LoRa] Requesting network time...");
            CommandSender_Send("AT+LTIME=?\r\n");
            ctx->state = LORA_STATE_WAIT_LTIME_RESPONSE;
            break;
        case LORA_STATE_WAIT_LTIME_RESPONSE:
            if (uart_rx) {
                LOG_DEBUG("[LoRa] LTIME response received: '%s'", uart_rx);
                
                // ResponseHandler에서 시간 응답 파싱 처리
                if (ResponseHandler_IsTimeResponse(uart_rx)) {
                    ResponseHandler_ParseTimeResponse(uart_rx);
                    
                    // 현재 상태에 따라 다른 동작
                    if (ctx->send_count == 0) {
                        // 첫 번째 시간 동기화 (JOIN 후) - 주기적 전송 시작
                        LOG_WARN("[LoRa] 🕐 Initial time synchronized, starting periodic transmission");
                        ctx->state = LORA_STATE_SEND_PERIODIC;
                        LOG_WARN("[LoRa] 🚀 PERIODIC SEND STARTED with message: %s", ctx->send_message);
                    } else {
                        // 주기적 전송 전 시간 조회 완료 - SEND 실행
                        LOG_WARN("[LoRa] 🕐 Time synchronized, proceeding to SEND");
                        ctx->state = LORA_STATE_SEND_PERIODIC;
                        // 시간은 다음 전송 후에 저장됨
                    }
                } else {
                    LOG_DEBUG("[LoRa] Waiting for LTIME response, got: '%s'", uart_rx);
                }
            } else {
                LOG_DEBUG("[LoRa] WAIT_LTIME_RESPONSE: No uart_rx data received");
            }
            break;
        case LORA_STATE_SEND_PERIODIC:
            {
                char send_cmd[128];
                char hex_data[64];
                const char* message = (ctx->send_message != NULL) ? ctx->send_message : "Hello";
                
                // 문자열을 헥사 문자열로 변환
                int len = strlen(message);
                for (int i = 0; i < len && i < 31; i++) {  // 최대 31자 (62 hex chars)
                    sprintf(&hex_data[i*2], "%02X", (unsigned char)message[i]);
                }
                hex_data[len*2] = '\0';
                
                snprintf(send_cmd, sizeof(send_cmd), "AT+SEND=1:%s\r\n", hex_data);
                LORA_LOG_SEND_ATTEMPT(message);
                CommandSender_Send(send_cmd);
                ctx->state = LORA_STATE_WAIT_SEND_RESPONSE;
                ctx->send_count++;
                LOG_DEBUG("[LoRa] Send count: %d", ctx->send_count);
            }
            break;
        case LORA_STATE_WAIT_SEND_RESPONSE:
            if (uart_rx) {
                ResponseType response_type = ResponseHandler_ParseSendResponse(uart_rx);
                switch(response_type) {
                    case RESPONSE_OK:
                        LORA_LOG_SEND_SUCCESS();
                        // SEND 성공 후 다음 전송 대기 상태로 전환
                        ctx->state = LORA_STATE_WAIT_SEND_INTERVAL;
                        ctx->error_count = 0; // 성공 시 에러 카운터 리셋
                        ctx->retry_delay_ms = 1000; // 재시도 지연 시간 리셋
                        ctx->last_send_time = TIME_GetCurrentMs(); // 송신 완료 시간 저장
                        LOG_INFO("[LoRa] SEND successful, waiting for next interval...");
                        break;
                    case RESPONSE_TIMEOUT:
                        LOG_WARN("[LoRa] SEND timeout - waiting for next interval");
                        ctx->state = LORA_STATE_WAIT_SEND_INTERVAL; // 타임아웃 시 대기 상태로 전환
                        ctx->error_count = 0; 
                        ctx->retry_delay_ms = 1000;
                        ctx->last_send_time = TIME_GetCurrentMs(); // 타임아웃 시간 저장
                        break;
                    case RESPONSE_ERROR:
                        LORA_LOG_SEND_FAILED("Network error");
                        ctx->error_count++;
                        LORA_LOG_ERROR_COUNT(ctx->error_count);
                        // 무제한 재시도 (max_retry_count가 0이거나 아직 제한에 도달하지 않은 경우)
                        if (ctx->max_retry_count == 0 || ctx->error_count < ctx->max_retry_count) {
                            LORA_LOG_RETRY_ATTEMPT(ctx->error_count, ctx->max_retry_count);
                            ctx->state = LORA_STATE_JOIN_RETRY; // JOIN 재시도
                        } else {
                            LORA_LOG_MAX_RETRIES_REACHED();
                            ctx->state = LORA_STATE_ERROR; // 최대 재시도 횟수 초과
                        }
                        break;
                    default:
                        // 알 수 없는 응답은 무시하고 계속 대기
                        LOG_DEBUG("[LoRa] Unknown response: %s", uart_rx);
                        break;
                }
            }
            break;
        case LORA_STATE_WAIT_SEND_INTERVAL:
            {
                uint32_t current_time = TIME_GetCurrentMs();
                uint32_t interval_ms = (ctx->send_interval_ms > 0) ? ctx->send_interval_ms : 30000; // 기본값 30초
                
                if ((current_time - ctx->last_send_time) >= interval_ms) {
                    LOG_DEBUG("[LoRa] Send interval passed (%u ms), requesting time before next send", interval_ms);
                    // 다음 주기적 전송 전 시간 동기화 실행 (LTIME → SEND 순서)
                    ctx->state = LORA_STATE_SEND_LTIME;
                } else {
                    // 아직 대기 시간이 남았으므로 상태 유지
                    uint32_t remaining_ms = interval_ms - (current_time - ctx->last_send_time);
                    LOG_DEBUG("[LoRa] Waiting for send interval (%u ms remaining)", remaining_ms);
                }
            }
            break;
        case LORA_STATE_JOIN_RETRY:
            {
                uint32_t current_time = TIME_GetCurrentMs();
                
                if (ctx->last_retry_time == 0) {
                    // 첫 재시도: 바로 SEND_JOIN
                    LOG_DEBUG("[LoRa] First JOIN retry");
                    ctx->last_retry_time = current_time;
                    ctx->state = LORA_STATE_SEND_JOIN;
                } else if ((current_time - ctx->last_retry_time) >= ctx->retry_delay_ms) {
                    // 지연 시간이 지났으면 SEND_JOIN
                    LOG_DEBUG("[LoRa] JOIN retry after %lu ms delay", ctx->retry_delay_ms);
                    ctx->last_retry_time = current_time;
                    ctx->state = LORA_STATE_SEND_JOIN;
                } else {
                    // 아직 지연 시간이 지나지 않았다면 상태 유지
                    LOG_DEBUG("[LoRa] Waiting for retry delay (%lu ms remaining)", 
                             ctx->retry_delay_ms - (current_time - ctx->last_retry_time));
                    // 아무것도 하지 않음
                }
            }
            break;
        case LORA_STATE_DONE:
        case LORA_STATE_ERROR:
        default:
            // 이미 완료된 상태이므로 아무것도 하지 않음
            break;
    }

    // 상태 변경 로깅
    if (old_state != ctx->state) {
        LORA_LOG_STATE_CHANGE(get_state_name(old_state), get_state_name(ctx->state));
    }
}
