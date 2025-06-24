#include <stdint.h>
// [임시 MOCK] 테스트 환경에서 TIME_GetCurrentMs 링크 오류 방지용. 실제 시간 흐름 테스트 필요시 개선 필요!
uint32_t TIME_GetCurrentMs(void) { return 0; }
#include "time_mock.c"

#ifdef TEST

#include "unity.h"
#include "mock_uart.h"
#include "mock_CommandSender.h"
#include "mock_ResponseHandler.h"

#include "LoraStarter.h"

void setUp(void)
{
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

#endif // TEST
