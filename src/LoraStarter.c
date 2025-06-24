#include "LoraStarter.h"
#include "uart.h"
#include "CommandSender.h"
#include "ResponseHandler.h"
#include "time.h"
#include <stddef.h>
#include <stdio.h>

void LoraStarter_ConnectUART(const char* port)
{
    UART_Connect(port);
}

void LoraStarter_Process(LoraStarterContext* ctx, const char* uart_rx)
{
    if (ctx == NULL) return;

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
            break;
        case LORA_STATE_SEND_CMD:
            if (ctx->cmd_index < ctx->num_commands) {
                CommandSender_Send(ctx->commands[ctx->cmd_index]);
                ctx->state = LORA_STATE_WAIT_OK;
            } else {
                ctx->state = LORA_STATE_SEND_JOIN;
            }
            break;
        case LORA_STATE_WAIT_OK:
            if (uart_rx && is_response_ok(uart_rx)) {
                ctx->cmd_index++;
                ctx->state = LORA_STATE_SEND_CMD;
            }
            break;
        case LORA_STATE_SEND_JOIN:
            CommandSender_Send("AT+JOIN");
            ctx->state = LORA_STATE_WAIT_JOIN_OK;
            break;
        case LORA_STATE_WAIT_JOIN_OK:
            if (uart_rx && is_join_response_ok(uart_rx)) {
                ctx->state = LORA_STATE_SEND_PERIODIC;
                ctx->send_count = 0;
                ctx->error_count = 0; // JOIN 성공 시 에러 카운터 리셋
                ctx->retry_delay_ms = 1000; // 재시도 지연 시간 리셋
                ctx->last_retry_time = 0; // 재시도 시간 리셋
            }
            break;
        case LORA_STATE_SEND_PERIODIC:
            {
                char send_cmd[128];
                const char* message = (ctx->send_message != NULL) ? ctx->send_message : "Hello";
                snprintf(send_cmd, sizeof(send_cmd), "AT+SEND=%s", message);
                CommandSender_Send(send_cmd);
                ctx->state = LORA_STATE_WAIT_SEND_RESPONSE;
                ctx->send_count++;
            }
            break;
        case LORA_STATE_WAIT_SEND_RESPONSE:
            if (uart_rx) {
                ResponseType response_type = ResponseHandler_ParseSendResponse(uart_rx);
                switch(response_type) {
                    case RESPONSE_OK:
                    case RESPONSE_TIMEOUT:
                        ctx->state = LORA_STATE_SEND_PERIODIC; // 주기적 송신 계속
                        ctx->error_count = 0; // 성공 시 에러 카운터 리셋
                        ctx->retry_delay_ms = 1000; // 재시도 지연 시간 리셋
                        break;
                    case RESPONSE_ERROR:
                        ctx->error_count++;
                        // 무제한 재시도 (max_retry_count가 0이거나 아직 제한에 도달하지 않은 경우)
                        if (ctx->max_retry_count == 0 || ctx->error_count < ctx->max_retry_count) {
                            ctx->state = LORA_STATE_JOIN_RETRY; // JOIN 재시도
                        } else {
                            ctx->state = LORA_STATE_ERROR; // 최대 재시도 횟수 초과
                        }
                        break;
                    default:
                        // 알 수 없는 응답은 무시하고 계속 대기
                        break;
                }
            }
            break;
        case LORA_STATE_JOIN_RETRY:
            {
                // 지수 백오프 적용: 1초, 2초, 4초, 8초, 16초... (최대 60초)
                uint32_t current_time = TIME_GetCurrentMs();
                
                // 첫 번째 재시도이거나 지연 시간이 지났으면 재시도
                if (ctx->last_retry_time == 0 || 
                    (current_time - ctx->last_retry_time) >= ctx->retry_delay_ms) {
                    
                    // 지수 백오프 계산 (최대 60초로 제한)
                    ctx->retry_delay_ms = 1000 * (1 << ctx->error_count);
                    if (ctx->retry_delay_ms > 60000) {
                        ctx->retry_delay_ms = 60000; // 최대 60초
                    }
                    
                    ctx->last_retry_time = current_time;
                    ctx->state = LORA_STATE_SEND_JOIN;
                }
                // 아직 지연 시간이 지나지 않았으면 대기 상태 유지
            }
            break;
        case LORA_STATE_DONE:
        case LORA_STATE_ERROR:
        default:
            // 이미 완료된 상태이므로 아무것도 하지 않음
            break;
    }
}
