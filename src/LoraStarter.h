#ifndef LORASTARTER_H
#define LORASTARTER_H

typedef enum {
    LORA_STATE_INIT,
    LORA_STATE_SEND_CMD,
    LORA_STATE_WAIT_OK,
    LORA_STATE_SEND_JOIN,
    LORA_STATE_WAIT_JOIN_OK,
    LORA_STATE_SEND_PERIODIC,      // 주기적 송신 상태
    LORA_STATE_WAIT_SEND_RESPONSE, // 송신 응답 대기
    LORA_STATE_JOIN_RETRY,         // JOIN 재시도 (ERROR 시)
    LORA_STATE_DONE,
    LORA_STATE_ERROR
} LoraState;

typedef struct {
    LoraState state;
    int cmd_index;
    const char** commands;
    int num_commands;
    unsigned long last_send_time;   // 마지막 송신 시간
    unsigned long send_interval_ms; // 송신 주기 (밀리초)
    int send_count;                 // 송신 횟수 (테스트용)
} LoraStarterContext;

void LoraStarter_ConnectUART(const char* port);
void LoraStarter_Process(LoraStarterContext* ctx, const char* uart_rx);

#endif // LORASTARTER_H
