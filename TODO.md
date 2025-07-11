## �� **심각한 문제들**

### 1. **LoraStarter.c - 하드코딩된 값들**
- **라인 47**: `ctx->retry_delay_ms = 1000;` - 재시도 지연 시간이 하드코딩됨
- **라인 46**: `if (ctx->send_message == NULL) ctx->send_message = "Hello";` - 기본 메시지가 하드코딩됨
- **라인 45**: `if (ctx->max_retry_count == 0) ctx->max_retry_count = 0;` - 의미 없는 코드 (무제한을 0으로 설정하는데 다시 0으로 설정)
- **라인 82**: `ctx->retry_delay_ms = 1000;` - JOIN 성공 시 재시도 지연 시간 리셋이 하드코딩됨
- **라인 95**: `const char* message = (ctx->send_message != NULL) ? ctx->send_message : "Hello";` - 기본 메시지가 하드코딩됨
- **라인 108, 113**: `ctx->retry_delay_ms = 1000;` - 성공/타임아웃 시 재시도 지연 시간 리셋이 하드코딩됨

### 2. **LoraStarter.c - 디버그 코드 (프로덕션에 남아있음)**
- **라인 125-127**: `printf("DEBUG: JOIN_RETRY - ...")` - 디버그 출력이 프로덕션 코드에 남아있음
- **라인 131, 136, 141**: `printf("DEBUG: JOIN_RETRY - Branch ...")` - 디버그 출력이 프로덕션 코드에 남아있음

### 3. **test_LoraStarter.c - 임시 워크어라운드**
- **라인 1-12**: TIME_GetCurrentMs 함수 재정의 - 테스트 환경에서만 사용되는 임시 조치
- **라인 2-3, 11-12**: 주석으로 표시된 임시 워크어라운드

### 4. **uart_common.c - 하드코딩된 값들**
- **라인 130**: `TIME_DelayMs(10);` - CPU 사용률을 줄이기 위한 지연 시간이 하드코딩됨

### 5. **uart.h - 하드코딩된 기본값들**
- **라인 22-26**: 모든 UART 기본 설정이 하드코딩됨
  - `UART_DEFAULT_BAUD_RATE 115200`
  - `UART_DEFAULT_DATA_BITS 8`
  - `UART_DEFAULT_STOP_BITS 1`
  - `UART_DEFAULT_PARITY 0`
  - `UART_DEFAULT_TIMEOUT_MS 1000`

### 6. **ResponseHandler.c - 하드코딩된 문자열들**
- **라인 15, 19, 23**: "OK", "OK\r\n", "OK\n" - 응답 문자열이 하드코딩됨
- **라인 40**: "+EVT:JOINED" - JOIN 응답 문자열이 하드코딩됨
- **라인 60**: "+EVT:SEND_CONFIRMED_OK" - SEND 성공 응답이 하드코딩됨
- **라인 64**: "+EVT:SEND_CONFIRMED_FAILED" - SEND 실패 응답이 하드코딩됨
- **라인 68**: "TIMEOUT" - 타임아웃 응답이 하드코딩됨

### 7. **LoraStarter.c - 하드코딩된 AT 명령어들**
- **라인 70**: `CommandSender_Send("AT+JOIN");` - JOIN 명령어가 하드코딩됨
- **라인 94**: `snprintf(send_cmd, sizeof(send_cmd), "AT+SEND=%s", message);` - SEND 명령어 형식이 하드코딩됨

### 8. **project.yml - 하드코딩된 빌드 설정**
- **라인 115**: `# - src/time_common.c` - time_common.c가 주석 처리되어 있음 (테스트용 time_mock.c만 사용)
- **라인 113**: `- src/time_mock.c` - 테스트용 mock이 프로덕션 빌드에 포함됨

### 9. **logger.c - 하드코딩된 기본값들**
- **라인 8-13**: LoggerConfig의 기본값들이 하드코딩됨
- **라인 35**: `char formatted[256];` - 로그 버퍼 크기가 하드코딩됨
- **라인 36**: `const char* level_str[] = {"[DEBUG]", "[INFO]", "[WARN]", "[ERROR]"};` - 로그 레벨 문자열이 하드코딩됨

### 10. **uart_mock.c - 하드코딩된 버퍼 크기**
- **라인 6**: `static char mock_receive_buffer[1024];` - Mock 버퍼 크기가 하드코딩됨
- **라인 10**: `static char delayed_response_buffer[1024];` - 지연 응답 버퍼 크기가 하드코딩됨

## 🟡 **개선이 필요한 부분들**

### 1. **LoraStarter.h - 설정 구조체 부족**
- 재시도 지연 시간, 기본 메시지, 최대 재시도 횟수 등을 설정할 수 있는 구조체가 없음

### 2. **logger_platform.c - 더미 구현**
- 모든 함수가 단순히 OK를 반환하는 더미 구현

### 3. **time_mock.c - 프로덕션 코드에 포함**
- 테스트용 mock이 프로덕션 빌드에 포함되어 있음

이러한 문제들은 코드의 유지보수성, 확장성, 그리고 실제 하드웨어에서의 동작에 영향을 줄 수 있습니다. 특히 하드코딩된 값들은 설정 파일이나 매개변수로 외부화되어야 하며, 디버그 코드는 제거되어야 합니다.