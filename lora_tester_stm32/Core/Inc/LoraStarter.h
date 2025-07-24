#ifndef LORASTARTER_H
#define LORASTARTER_H

// LoRa 기본 초기화 명령어 배열 (TDD 검증됨)
extern const char* LORA_DEFAULT_INIT_COMMANDS[];
extern const int LORA_DEFAULT_INIT_COMMANDS_COUNT;

typedef enum {
    LORA_STATE_INIT,
    LORA_STATE_SEND_CMD,
    LORA_STATE_WAIT_OK,
    LORA_STATE_SEND_JOIN,
    LORA_STATE_WAIT_JOIN_OK,
    LORA_STATE_SEND_PERIODIC,      // 주기적 송신 상태
    LORA_STATE_WAIT_SEND_RESPONSE, // 송신 응답 대기
    LORA_STATE_WAIT_SEND_INTERVAL, // 주기적 송신 대기 (타이머)
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
    const char* send_message;       // 송신할 메시지 (하드코딩 제거)
    int error_count;                // 에러 발생 횟수
    int max_retry_count;            // 최대 재시도 횟수 (0이면 무제한)
    unsigned long last_retry_time;  // 마지막 재시도 시간
    unsigned long retry_delay_ms;   // 현재 재시도 지연 시간
} LoraStarterContext;

void LoraStarter_ConnectUART(const char* port);
void LoraStarter_Process(LoraStarterContext* ctx, const char* uart_rx);

// 편의 함수: 기본 설정으로 LoraStarter 컨텍스트 초기화
void LoraStarter_InitWithDefaults(LoraStarterContext* ctx, const char* send_message);

#endif // LORASTARTER_H
