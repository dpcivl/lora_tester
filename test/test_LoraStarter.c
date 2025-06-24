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
    CommandSender_Send_Expect("AT+SEND=Hello");
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
    CommandSender_Send_Expect("AT+SEND=Hello");
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);

    // 2. OK 응답 수신 시 다시 주기적 송신 상태로 전이
    ResponseHandler_ParseSendResponse_ExpectAndReturn("+EVT:SEND_CONFIRMED_OK", RESPONSE_OK);
    LoraStarter_Process(&ctx, "+EVT:SEND_CONFIRMED_OK"); // WAIT_SEND_RESPONSE -> SEND_PERIODIC
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);

    // 3. 두 번째 send 명령어 전송
    CommandSender_Send_Expect("AT+SEND=Hello");
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
    CommandSender_Send_Expect("AT+SEND=Hello");
    LoraStarter_Process(&ctx, NULL); // SEND_PERIODIC -> WAIT_SEND_RESPONSE
    TEST_ASSERT_EQUAL(LORA_STATE_WAIT_SEND_RESPONSE, ctx.state);

    // 2. TIMEOUT 응답 수신 시 다시 주기적 송신 상태로 전이
    ResponseHandler_ParseSendResponse_ExpectAndReturn("TIMEOUT", RESPONSE_TIMEOUT);
    LoraStarter_Process(&ctx, "TIMEOUT"); // WAIT_SEND_RESPONSE -> SEND_PERIODIC
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_PERIODIC, ctx.state);

    // 3. 두 번째 send 명령어 전송
    CommandSender_Send_Expect("AT+SEND=Hello");
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
    CommandSender_Send_Expect("AT+SEND=Hello");
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
    CommandSender_Send_Expect("AT+SEND=Hello");
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
    const char* commands[] = {"AT+NWM=1"};
    LoraStarterContext ctx = {
        .state = LORA_STATE_JOIN_RETRY,
        .cmd_index = 0,
        .commands = commands,
        .num_commands = 1,
        .last_retry_time = 1000,  // 1초 전에 재시도
        .retry_delay_ms = 1000    // 1초 지연 필요
    };
    // Set mock time to 2000 (1000ms passed, delay has passed)
    TIME_Mock_SetCurrentTime(2000);
    LoraStarter_Process(&ctx, NULL);
    // Should proceed to SEND_JOIN because delay has passed
    TEST_ASSERT_EQUAL(LORA_STATE_SEND_JOIN, ctx.state);
}

#endif // TEST
