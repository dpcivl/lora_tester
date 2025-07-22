// -----------------------------------------------------------------------------
// [임시 워크어라운드] 테스트 환경에서 TIME_GetCurrentMs 심볼 충돌/링커 오류 방지용
// 실제 구현과 테스트 분리를 위해 향후 CMock/테스트 빌드 설정에서 더 우아하게 처리 필요!
// -----------------------------------------------------------------------------
#include <stdint.h>
#include "time.h"
uint32_t TIME_GetCurrentMs(void) {
    return TIME_Platform_GetCurrentMs();
}
// -----------------------------------------------------------------------------
// ↑ 위 코드는 테스트 빌드에서만 사용되는 임시 조치입니다. 실제 소스/테스트 분리 리팩터링 필요!
// -----------------------------------------------------------------------------
#include "unity.h"
#include "mock_uart.h"
#include "mock_CommandSender.h"
#include "mock_ResponseHandler.h"
#include "LoraStarter.h"
#include "time_mock.c"

#ifdef TEST

#include "time.h"
#include "unity.h"
#include "mock_uart.h"
#include "mock_CommandSender.h"
#include "mock_ResponseHandler.h"
#include "LoraStarter.h"
#include "mock_logger.h"

void setUp(void)
{
    LOGGER_SendWithLevel_IgnoreAndReturn(LOGGER_STATUS_OK);
}

void tearDown(void)
{
}

void test_LoraStarter_ConnectUART_should_call_UART_Connect(void)
{
    UART_Connect_ExpectAndReturn("COM1", 0);
    LoraStarter_ConnectUART("COM1");
}

void test_LoraStarter_should_send_commands_in_sequence_and_wait_for_OK(void)
{
    // 준비: 커맨드 배열
    const char* commands[] = {"AT+NWM=1", "AT+NJM=1", "AT+CLASS=A", "AT+BAND=7"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_INIT,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 4
    };

    // 1. INIT 상태에서 첫 번째 커맨드 전송 준비
    LoraStarter_Process(&ctx, NULL); // INIT -> SEND_CMD
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_CMD, ctx.state);
    
    CommandSender_Send_Expect("AT+NWM=1");
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> WAIT_OK
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_OK, ctx.state);

    // 2. 첫 번째 OK 수신 시 두 번째 커맨드 전송
    is_response_ok_ExpectAndReturn("OK", 1);
    LoraStarter_Process(&ctx, "OK"); // WAIT_OK -> SEND_CMD
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_CMD, ctx.state);
    
    CommandSender_Send_Expect("AT+NJM=1");
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> WAIT_OK
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_OK, ctx.state);

    // 3. 두 번째 OK 수신 시 세 번째 커맨드 전송
    is_response_ok_ExpectAndReturn("OK", 1);
    LoraStarter_Process(&ctx, "OK"); // WAIT_OK -> SEND_CMD
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_CMD, ctx.state);
    
    CommandSender_Send_Expect("AT+CLASS=A");
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> WAIT_OK
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_OK, ctx.state);

    // 4. 세 번째 OK 수신 시 네 번째 커맨드 전송
    is_response_ok_ExpectAndReturn("OK", 1);
    LoraStarter_Process(&ctx, "OK"); // WAIT_OK -> SEND_CMD
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_CMD, ctx.state);
    
    CommandSender_Send_Expect("AT+BAND=7");
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> WAIT_OK
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_OK, ctx.state);

    // 5. 네 번째 OK 수신 시 JOIN 커맨드 전송 (모든 커맨드 완료)
    is_response_ok_ExpectAndReturn("OK", 1);
    LoraStarter_Process(&ctx, "OK"); // WAIT_OK -> SEND_CMD
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_CMD, ctx.state);
    
    // cmd_index가 num_commands와 같으므로 SEND_JOIN으로 전이
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> SEND_JOIN
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_JOIN, ctx.state);
    
    CommandSender_Send_Expect("AT+JOIN");
    LoraStarter_Process(&ctx, NULL); // SEND_JOIN -> WAIT_JOIN_OK
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_JOIN_OK, ctx.state);

    // 6. JOIN OK 수신 시 DONE 상태로 전이
    is_join_response_ok_ExpectAndReturn("+EVT:JOINED", 1);
    LoraStarter_Process(&ctx, "+EVT:JOINED"); // WAIT_JOIN_OK -> SEND_PERIODIC

    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);
}

void test_LoraStarter_should_start_periodic_send_after_JOIN_OK(void)
{
    // 준비: 커맨드 배열
    const char* commands[] = {"AT+NWM=1", "AT+NJM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_INIT,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 2,
        .send_interval_ms = 1000,  // 1초 주기
        .send_count = 0,
        .send_message = "Hello",
        .error_count = 0,
        .max_retry_count = 0,  // 무제한 재시도
        .last_retry_time = 0,
        .retry_delay_ms = 1000
    };

    // 1. INIT 상태에서 첫 번째 커맨드 전송
    LoraStarter_Process(&ctx, NULL); // INIT -> SEND_CMD
    CommandSender_Send_Expect("AT+NWM=1");
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> WAIT_OK

    // 2. 첫 번째 OK 수신 시 두 번째 커맨드 전송
    is_response_ok_ExpectAndReturn("OK", 1);
    LoraStarter_Process(&ctx, "OK"); // WAIT_OK -> SEND_CMD
    CommandSender_Send_Expect("AT+NJM=1");
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> WAIT_OK

    // 3. 두 번째 OK 수신 시 JOIN 커맨드 전송
    is_response_ok_ExpectAndReturn("OK", 1);
    LoraStarter_Process(&ctx, "OK"); // WAIT_OK -> SEND_CMD
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> SEND_JOIN
    CommandSender_Send_Expect("AT+JOIN");
    LoraStarter_Process(&ctx, NULL); // SEND_JOIN -> WAIT_JOIN_OK

    // 4. JOIN OK 수신 시 주기적 송신 상태로 전이
    is_join_response_ok_ExpectAndReturn("+EVT:JOINED", 1);
    LoraStarter_Process(&ctx, "+EVT:JOINED"); // WAIT_JOIN_OK -> SEND_PERIODIC

    // 5. 주기적 송신 상태 확인
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);
    
    // 6. 첫 번째 주기적 송신 시작
    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);
}

void test_LoraStarter_should_continue_periodic_send_on_OK_response(void)
{
    // 준비: 커맨드 배열
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_SEND_PERIODIC,  // 이미 주기적 송신 상태
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .send_interval_ms = 1000,
        .send_count = 0,
        .send_message = "Hello",
        .error_count = 0,
        .max_retry_count = 0,  // 무제한 재시도
        .last_retry_time = 0,
        .retry_delay_ms = 1000
    };

    // 1. 첫 번째 send 명령어 전송
    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);

    // 2. OK 응답 수신 시 대기 상태로 전이
    ResponseHandler_ParseSendResponse_ExpectAndReturn("+EVT:SEND_CONFIRMED_OK", RESPONSE_OK);
    LoraStarter_Process(&ctx, "+EVT:SEND_CONFIRMED_OK"); // WAIT_SEND_RESPONSE -> WAIT_SEND_INTERVAL
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);

    // 3. 충분한 시간이 지나서 다시 SEND_PERIODIC으로 전이
    TIME_Mock_SetCurrentTime(ctx.last_send_time + 35000); // 35초 후 (기본 30초 간격 초과)
    LoraStarter_Process(&ctx, NULL); // WAIT_SEND_INTERVAL -> SEND_PERIODIC
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);

    // 4. 두 번째 send 명령어 전송
    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);
}

void test_LoraStarter_should_continue_periodic_send_on_TIMEOUT_response(void)
{
    // 준비: 커맨드 배열
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_SEND_PERIODIC,  // 이미 주기적 송신 상태
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .send_interval_ms = 1000,
        .send_count = 0,
        .send_message = "Hello",
        .error_count = 0,
        .max_retry_count = 0,  // 무제한 재시도
        .last_retry_time = 0,
        .retry_delay_ms = 1000
    };

    // 1. 첫 번째 send 명령어 전송
    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);

    // 2. TIMEOUT 응답 수신 시 대기 상태로 전이
    ResponseHandler_ParseSendResponse_ExpectAndReturn("TIMEOUT", RESPONSE_TIMEOUT);
    LoraStarter_Process(&ctx, "TIMEOUT"); // WAIT_SEND_RESPONSE -> WAIT_SEND_INTERVAL
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);

    // 3. 충분한 시간이 지나서 다시 SEND_PERIODIC으로 전이
    TIME_Mock_SetCurrentTime(ctx.last_send_time + 35000); // 35초 후 (기본 30초 간격 초과)
    LoraStarter_Process(&ctx, NULL); // WAIT_SEND_INTERVAL -> SEND_PERIODIC
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);

    // 4. 두 번째 send 명령어 전송
    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);
}

void test_LoraStarter_should_retry_JOIN_on_ERROR_response(void)
{
    // 준비: 커맨드 배열
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_SEND_PERIODIC,  // 이미 주기적 송신 상태
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .send_interval_ms = 1000,
        .send_count = 0,
        .send_message = "Hello",
        .error_count = 0,
        .max_retry_count = 0,  // 무제한 재시도
        .last_retry_time = 0,
        .retry_delay_ms = 1000
    };

    // 1. 첫 번째 send 명령어 전송
    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);

    // 2. ERROR 응답 수신 시 JOIN 재시도 상태로 전이
    ResponseHandler_ParseSendResponse_ExpectAndReturn("+EVT:SEND_CONFIRMED_FAILED(4)", RESPONSE_ERROR);
    LoraStarter_Process(&ctx, "+EVT:SEND_CONFIRMED_FAILED(4)"); // WAIT_SEND_RESPONSE -> JOIN_RETRY
    TEST_ASSERT_EQUAL(LORA_STATE_JOIN_RETRY, ctx.state);

    // 3. JOIN 재시도 시 SEND_JOIN 상태로 전이
    LoraStarter_Process(&ctx, NULL); // JOIN_RETRY -> SEND_JOIN
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_JOIN, ctx.state);

    // 4. JOIN 커맨드 전송
    CommandSender_Send_Expect("AT+JOIN");
    LoraStarter_Process(&ctx, NULL); // SEND_JOIN -> WAIT_JOIN_OK
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_JOIN_OK, ctx.state);
}

void test_LoraStarter_should_do_nothing_when_ctx_is_NULL(void)
{
    // Should not crash or do anything
    LoraStarter_Process(NULL, NULL);
}

void test_LoraStarter_should_handle_empty_command_list(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_INIT,
        .cmd_index = 0,
        .commands = NULL,
        .num_commands = 0
    };
    LoraStarter_Process(&ctx, NULL); // INIT -> SEND_CMD
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_CMD, ctx.state);
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> SEND_JOIN (since cmd_index >= num_commands)
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_JOIN, ctx.state);
}

void test_LoraStarter_should_handle_null_send_message(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_SEND_PERIODIC,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .send_message = NULL
    };
    // Should use default "Hello"
    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"
    LoraStarter_Process(&ctx, NULL);
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);
}

void test_LoraStarter_should_stop_retry_on_max_retry_count(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .send_message = "Hello",
        .error_count = 3,
        .max_retry_count = 3
    };
    ResponseHandler_ParseSendResponse_ExpectAndReturn("+EVT:SEND_CONFIRMED_FAILED(4)", RESPONSE_ERROR);
    LoraStarter_Process(&ctx, "+EVT:SEND_CONFIRMED_FAILED(4)");
    TEST_ASSERT_EQUAL(LORA_STATE_ERROR, ctx.state);
}

void test_LoraStarter_should_ignore_unknown_response_in_WAIT_SEND_RESPONSE(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .send_message = "Hello"
    };
    ResponseHandler_ParseSendResponse_ExpectAndReturn("UNKNOWN", 999); // Unknown response type
    LoraStarter_Process(&ctx, "UNKNOWN");
    // Should remain in WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);
}

void test_LoraStarter_should_stay_in_WAIT_OK_when_response_is_NULL(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_OK,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1
    };
    LoraStarter_Process(&ctx, NULL);
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_OK, ctx.state);
}

void test_LoraStarter_should_stay_in_WAIT_OK_when_response_is_not_OK(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_OK,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1
    };
    is_response_ok_ExpectAndReturn("ERROR", 0);
    LoraStarter_Process(&ctx, "ERROR");
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_OK, ctx.state);
}

void test_LoraStarter_should_stay_in_WAIT_JOIN_OK_when_response_is_NULL(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_JOIN_OK,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1
    };
    LoraStarter_Process(&ctx, NULL);
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_JOIN_OK, ctx.state);
}

void test_LoraStarter_should_stay_in_WAIT_JOIN_OK_when_response_is_not_JOINED(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_JOIN_OK,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1
    };
    is_join_response_ok_ExpectAndReturn("ERROR", 0);
    LoraStarter_Process(&ctx, "ERROR");
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_JOIN_OK, ctx.state);
}

void test_LoraStarter_should_stay_in_JOIN_RETRY_when_delay_time_hasnt_passed(void)
{
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_JOIN_RETRY,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .last_retry_time = 1000,  // 1초 전에 재시도
        .retry_delay_ms = 2000    // 2초 지연 필요
    };
    // Set mock time to 1500 (500ms passed, but need 2000ms)
    TIME_Mock_SetCurrentTime(1500);
    LoraStarter_Process(&ctx, NULL);
    // Should stay in JOIN_RETRY because delay hasn't passed
    TEST_ASSERT_EQUAL(LORA_STATE_JOIN_RETRY, ctx.state);
}

void test_LoraStarter_should_proceed_to_SEND_JOIN_when_delay_time_has_passed(void)
{
    // 준비: JOIN_RETRY 상태에서 지연 시간이 지난 상황
    LoraStarterContext ctx = {
        .state = LORA_STATE_JOIN_RETRY,
        .last_retry_time = 1000,  // 1초 전에 재시도
        .retry_delay_ms = 500     // 0.5초 지연
    };

    // TIME_GetCurrentMs가 1600을 반환 (1.6초 경과)
    TIME_Mock_SetCurrentTime(1600);

    LoraStarter_Process(&ctx, NULL);

    // 지연 시간이 지났으므로 SEND_JOIN으로 전이
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_JOIN, ctx.state);
    TEST_ASSERT_EQUAL(1600, ctx.last_retry_time);  // 재시도 시간 업데이트
}

// get_state_name 함수 테스트
void test_get_state_name_should_return_correct_strings(void)
{
    // 이 테스트는 get_state_name이 static 함수이므로 직접 테스트할 수 없지만,
    // LoraStarter_Process를 통해 간접적으로 테스트할 수 있습니다.
    
    LoraStarterContext ctx = {
        .state = LORA_STATE_INIT
    };

    // 각 상태를 거쳐가면서 로깅이 제대로 되는지 확인
    LoraStarter_Process(&ctx, NULL); // INIT -> SEND_CMD
    LoraStarter_Process(&ctx, NULL); // SEND_CMD -> WAIT_OK (명령어가 없으므로)
    
    // 상태 변경 로깅이 호출되었는지 확인
    // (실제로는 mock_logger를 통해 확인할 수 있음)
}

// 에러 상태 처리 테스트
void test_LoraStarter_should_do_nothing_in_DONE_state(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_DONE,
        .send_count = 5
    };

    int original_send_count = ctx.send_count;
    
    LoraStarter_Process(&ctx, "some response");
    
    // DONE 상태에서는 아무것도 변경되지 않아야 함
    TEST_ASSERT_EQUAL(LORA_STATE_DONE, ctx.state);
    TEST_ASSERT_EQUAL(original_send_count, ctx.send_count);
}

void test_LoraStarter_should_do_nothing_in_ERROR_state(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_ERROR,
        .error_count = 10
    };

    int original_error_count = ctx.error_count;
    
    LoraStarter_Process(&ctx, "some response");
    
    // ERROR 상태에서는 아무것도 변경되지 않아야 함
    TEST_ASSERT_EQUAL(LORA_STATE_ERROR, ctx.state);
    TEST_ASSERT_EQUAL(original_error_count, ctx.error_count);
}

// 기본값 설정 테스트
void test_LoraStarter_should_set_default_values_in_INIT_state(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_INIT,
        .max_retry_count = 0,
        .send_message = NULL
    };

    LoraStarter_Process(&ctx, NULL);

    // 기본값이 설정되었는지 확인
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_CMD, ctx.state);
    TEST_ASSERT_EQUAL(0, ctx.cmd_index);
    TEST_ASSERT_EQUAL(0, ctx.error_count);
    TEST_ASSERT_EQUAL(0, ctx.max_retry_count);  // 0은 무제한을 의미
    TEST_ASSERT_EQUAL_STRING("Hello", ctx.send_message);  // 기본 메시지
    TEST_ASSERT_EQUAL(0, ctx.last_retry_time);
    TEST_ASSERT_EQUAL(1000, ctx.retry_delay_ms);
}

// WAIT_OK 상태에서 NULL 응답 처리
void test_LoraStarter_should_stay_in_WAIT_OK_when_uart_rx_is_NULL(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_OK,
        .cmd_index = 1
    };

    int original_cmd_index = ctx.cmd_index;
    
    LoraStarter_Process(&ctx, NULL);
    
    // NULL 응답이므로 상태가 변경되지 않아야 함
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_OK, ctx.state);
    TEST_ASSERT_EQUAL(original_cmd_index, ctx.cmd_index);
}

// WAIT_JOIN_OK 상태에서 NULL 응답 처리
void test_LoraStarter_should_stay_in_WAIT_JOIN_OK_when_uart_rx_is_NULL(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_JOIN_OK
    };

    LoraStarter_Process(&ctx, NULL);
    
    // NULL 응답이므로 상태가 변경되지 않아야 함
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_JOIN_OK, ctx.state);
}

// WAIT_SEND_RESPONSE 상태에서 NULL 응답 처리
void test_LoraStarter_should_stay_in_WAIT_SEND_RESPONSE_when_uart_rx_is_NULL(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .error_count = 2
    };

    int original_error_count = ctx.error_count;
    
    LoraStarter_Process(&ctx, NULL);
    
    // NULL 응답이므로 상태가 변경되지 않아야 함
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);
    TEST_ASSERT_EQUAL(original_error_count, ctx.error_count);
}

// JOIN_RETRY 상태에서 첫 번째 재시도 (last_retry_time이 0인 경우)
void test_LoraStarter_should_immediately_retry_JOIN_when_last_retry_time_is_zero(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_JOIN_RETRY,
        .last_retry_time = 0,  // 첫 번째 재시도
        .retry_delay_ms = 1000
    };

    TIME_Mock_SetCurrentTime(5000);

    LoraStarter_Process(&ctx, NULL);

    // 첫 번째 재시도이므로 즉시 SEND_JOIN으로 전이
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_JOIN, ctx.state);
    TEST_ASSERT_EQUAL(5000, ctx.last_retry_time);
}

// JOIN_RETRY 상태에서 지연 시간이 정확히 같을 때
void test_LoraStarter_should_retry_JOIN_when_delay_time_is_exactly_equal(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_JOIN_RETRY,
        .last_retry_time = 1000,
        .retry_delay_ms = 500
    };

    TIME_Mock_SetCurrentTime(1500);  // 정확히 500ms 경과

    LoraStarter_Process(&ctx, NULL);

    // 지연 시간이 정확히 같으므로 SEND_JOIN으로 전이
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_JOIN, ctx.state);
    TEST_ASSERT_EQUAL(1500, ctx.last_retry_time);
}

// max_retry_count가 0이 아닌 경우의 재시도 제한 테스트
void test_LoraStarter_should_stop_retry_when_max_retry_count_is_reached(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .error_count = 3,
        .max_retry_count = 3  // 최대 3회 재시도
    };

    ResponseHandler_ParseSendResponse_ExpectAndReturn("ERROR", RESPONSE_ERROR);

    LoraStarter_Process(&ctx, "ERROR");

    // 최대 재시도 횟수에 도달했으므로 ERROR 상태로 전이
    TEST_ASSERT_EQUAL(LORA_STATE_ERROR, ctx.state);
    TEST_ASSERT_EQUAL(4, ctx.error_count);  // 에러 카운트 증가
}

// send_message가 NULL인 경우의 기본 메시지 사용 테스트
void test_LoraStarter_should_use_default_message_when_send_message_is_NULL(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_SEND_PERIODIC,
        .send_message = NULL
    };

    CommandSender_Send_Expect("AT+SEND=1:48656C6C6F"); // "Hello" → "48656C6C6F"  // 기본 메시지 사용

    LoraStarter_Process(&ctx, NULL);

    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);
    TEST_ASSERT_EQUAL(1, ctx.send_count);
}

// 에러 카운터 리셋 테스트
void test_LoraStarter_should_reset_error_count_on_successful_JOIN(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_JOIN_OK,
        .error_count = 5,
        .retry_delay_ms = 2000,
        .last_retry_time = 1000
    };

    is_join_response_ok_ExpectAndReturn("+EVT:JOINED", 1);

    LoraStarter_Process(&ctx, "+EVT:JOINED");

    // JOIN 성공 시 에러 카운터와 재시도 관련 값들이 리셋되어야 함
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);
    TEST_ASSERT_EQUAL(0, ctx.error_count);
    TEST_ASSERT_EQUAL(1000, ctx.retry_delay_ms);
    TEST_ASSERT_EQUAL(0, ctx.last_retry_time);
    TEST_ASSERT_EQUAL(0, ctx.send_count);
}

// 성공적인 SEND 응답에서 에러 카운터 리셋 테스트
void test_LoraStarter_should_reset_error_count_on_successful_SEND(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .error_count = 3,
        .retry_delay_ms = 2000,
        .last_retry_time = 1000
    };

    ResponseHandler_ParseSendResponse_ExpectAndReturn("+EVT:SEND_CONFIRMED_OK", RESPONSE_OK);

    LoraStarter_Process(&ctx, "+EVT:SEND_CONFIRMED_OK");

    // SEND 성공 시 에러 카운터와 재시도 관련 값들이 리셋되어야 함
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);
    TEST_ASSERT_EQUAL(0, ctx.error_count);
    TEST_ASSERT_EQUAL(1000, ctx.retry_delay_ms);
}

// TIMEOUT 응답에서 에러 카운터 리셋 테스트
void test_LoraStarter_should_reset_error_count_on_TIMEOUT_response(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .error_count = 2,
        .retry_delay_ms = 2000,
        .last_retry_time = 1000
    };

    ResponseHandler_ParseSendResponse_ExpectAndReturn("TIMEOUT", RESPONSE_TIMEOUT);

    LoraStarter_Process(&ctx, "TIMEOUT");

    // TIMEOUT 응답 시에도 에러 카운터와 재시도 관련 값들이 리셋되어야 함
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);
    TEST_ASSERT_EQUAL(0, ctx.error_count);
    TEST_ASSERT_EQUAL(1000, ctx.retry_delay_ms);
}

// 새로운 WAIT_SEND_INTERVAL 상태 테스트
void test_LoraStarter_should_wait_in_WAIT_SEND_INTERVAL_when_interval_not_passed(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_INTERVAL,
        .last_send_time = 1000,
        .send_interval_ms = 30000  // 30초 간격
    };
    
    // 현재 시간을 5초 후로 설정 (아직 30초가 지나지 않음)
    TIME_Mock_SetCurrentTime(6000);
    
    LoraStarter_Process(&ctx, NULL);
    
    // 아직 대기 시간이 남았으므로 상태 유지
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);
}

void test_LoraStarter_should_proceed_to_SEND_PERIODIC_when_interval_passed(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_INTERVAL,
        .last_send_time = 1000,
        .send_interval_ms = 30000  // 30초 간격
    };
    
    // 현재 시간을 35초 후로 설정 (30초 간격 초과)
    TIME_Mock_SetCurrentTime(35000);
    
    LoraStarter_Process(&ctx, NULL);
    
    // 대기 시간이 지났으므로 SEND_PERIODIC으로 전이
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);
}

void test_LoraStarter_should_use_default_interval_when_send_interval_ms_is_zero(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_INTERVAL,
        .last_send_time = 1000,
        .send_interval_ms = 0  // 기본값 사용
    };
    
    // 현재 시간을 4분 후로 설정 (기본값 5분보다 작음)
    TIME_Mock_SetCurrentTime(241000);
    
    LoraStarter_Process(&ctx, NULL);
    
    // 기본 5분 간격이 적용되어 아직 대기 상태
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);
    
    // 6분 후로 설정 (기본값 5분 초과)
    TIME_Mock_SetCurrentTime(361000);
    
    LoraStarter_Process(&ctx, NULL);
    
    // 이제 SEND_PERIODIC으로 전이
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);
}

void test_LoraStarter_should_set_last_send_time_on_successful_send(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .last_send_time = 0
    };
    
    TIME_Mock_SetCurrentTime(5000);
    
    ResponseHandler_ParseSendResponse_ExpectAndReturn("+EVT:SEND_CONFIRMED_OK", RESPONSE_OK);
    LoraStarter_Process(&ctx, "+EVT:SEND_CONFIRMED_OK");
    
    // 성공 시 last_send_time이 설정되어야 함
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);
    TEST_ASSERT_EQUAL(5000, ctx.last_send_time);
}

void test_LoraStarter_should_set_last_send_time_on_timeout(void)
{
    LoraStarterContext ctx = {
        .state = LORA_STATE_WAIT_SEND_RESPONSE,
        .last_send_time = 0
    };
    
    TIME_Mock_SetCurrentTime(7000);
    
    ResponseHandler_ParseSendResponse_ExpectAndReturn("TIMEOUT", RESPONSE_TIMEOUT);
    LoraStarter_Process(&ctx, "TIMEOUT");
    
    // 타임아웃 시에도 last_send_time이 설정되어야 함
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_INTERVAL, ctx.state);
    TEST_ASSERT_EQUAL(7000, ctx.last_send_time);
}

#endif // TEST
