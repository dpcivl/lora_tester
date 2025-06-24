#include "LoraStarter.h"
#include "uart.h"
#include "CommandSender.h"
#include "ResponseHandler.h"
#include <stddef.h>

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
            ctx->state = LORA_STATE_SEND_CMD;
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
            }
            break;
        case LORA_STATE_SEND_PERIODIC:
            CommandSender_Send("AT+SEND=Hello");
            ctx->state = LORA_STATE_WAIT_SEND_RESPONSE;
            ctx->send_count++;
            break;
        case LORA_STATE_WAIT_SEND_RESPONSE:
            if (uart_rx) {
                ResponseType response_type = ResponseHandler_ParseSendResponse(uart_rx);
                switch(response_type) {
                    case RESPONSE_OK:
                    case RESPONSE_TIMEOUT:
                        ctx->state = LORA_STATE_SEND_PERIODIC; // 주기적 송신 계속
                        break;
                    case RESPONSE_ERROR:
                        ctx->state = LORA_STATE_JOIN_RETRY; // JOIN 재시도
                        break;
                    default:
                        // 알 수 없는 응답은 무시하고 계속 대기
                        break;
                }
            }
            break;
        case LORA_STATE_JOIN_RETRY:
            ctx->state = LORA_STATE_SEND_JOIN;
            break;
        case LORA_STATE_DONE:
        case LORA_STATE_ERROR:
        default:
            // 이미 완료된 상태이므로 아무것도 하지 않음
            break;
    }
}
