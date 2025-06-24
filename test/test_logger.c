#ifdef TEST

#include "unity.h"
#include "mock_logger_platform.h"
#include "logger.h"

void setUp(void)
{
    LOGGER_Platform_Connect_ExpectAndReturn("127.0.0.1", 1234, LOGGER_STATUS_OK);
    LOGGER_Connect("127.0.0.1", 1234);
}
void tearDown(void) {}

void test_LOGGER_Send_should_call_platform_send(void)
{
    LOGGER_Platform_Send_ExpectAndReturn("Hello, world!", LOGGER_STATUS_OK);
    TEST_ASSERT_EQUAL(LOGGER_STATUS_OK, LOGGER_Send("Hello, world!"));
}

void test_LOGGER_Send_should_return_error_on_null_message(void)
{
    TEST_ASSERT_EQUAL(LOGGER_STATUS_ERROR, LOGGER_Send(NULL));
}

void test_LOGGER_Send_should_return_error_when_not_connected(void)
{
    // 연결 해제
    LOGGER_Platform_Disconnect_ExpectAndReturn(LOGGER_STATUS_OK);
    LOGGER_Disconnect();
    
    // 연결되지 않은 상태에서 메시지 전송 시도
    TEST_ASSERT_EQUAL(LOGGER_STATUS_ERROR, LOGGER_Send("Should fail"));
    
    // 다시 연결 (다음 테스트를 위해)
    LOGGER_Platform_Connect_ExpectAndReturn("127.0.0.1", 1234, LOGGER_STATUS_OK);
    LOGGER_Connect("127.0.0.1", 1234);
}

void test_LOGGER_SendWithLevel_should_filter_by_level(void)
{
    // 기본 레벨은 INFO, DEBUG는 무시됨
    LOGGER_Platform_Send_ExpectAndReturn("[INFO] Info message", LOGGER_STATUS_OK);
    TEST_ASSERT_EQUAL(LOGGER_STATUS_OK, LOGGER_SendWithLevel(LOG_LEVEL_INFO, "Info message"));
    // DEBUG 레벨은 무시 (플랫폼 함수 호출 안 됨)
    TEST_ASSERT_EQUAL(LOGGER_STATUS_OK, LOGGER_SendWithLevel(LOG_LEVEL_DEBUG, "Debug message"));
}

void test_LOGGER_SendWithLevel_should_handle_null_message(void)
{
    // NULL 메시지 처리 테스트
    LOGGER_Platform_Send_ExpectAndReturn("[INFO] ", LOGGER_STATUS_OK);
    TEST_ASSERT_EQUAL(LOGGER_STATUS_OK, LOGGER_SendWithLevel(LOG_LEVEL_INFO, NULL));
}

void test_LOGGER_IsConnected_should_return_true_when_connected(void)
{
    // 이미 setUp에서 연결됨
    TEST_ASSERT_TRUE(LOGGER_IsConnected());
}

void test_LOGGER_IsConnected_should_return_false_when_disconnected(void)
{
    // 연결 해제
    LOGGER_Platform_Disconnect_ExpectAndReturn(LOGGER_STATUS_OK);
    LOGGER_Disconnect();
    
    TEST_ASSERT_FALSE(LOGGER_IsConnected());
    
    // 다시 연결 (다음 테스트를 위해)
    LOGGER_Platform_Connect_ExpectAndReturn("127.0.0.1", 1234, LOGGER_STATUS_OK);
    LOGGER_Connect("127.0.0.1", 1234);
}

void test_LOGGER_Disconnect_should_call_platform_disconnect(void)
{
    LOGGER_Platform_Disconnect_ExpectAndReturn(LOGGER_STATUS_OK);
    TEST_ASSERT_EQUAL(LOGGER_STATUS_OK, LOGGER_Disconnect());
    
    // 다시 연결 (다음 테스트를 위해)
    LOGGER_Platform_Connect_ExpectAndReturn("127.0.0.1", 1234, LOGGER_STATUS_OK);
    LOGGER_Connect("127.0.0.1", 1234);
}

void test_LOGGER_Disconnect_should_return_ok_when_not_connected(void)
{
    // 연결 해제
    LOGGER_Platform_Disconnect_ExpectAndReturn(LOGGER_STATUS_OK);
    LOGGER_Disconnect();
    
    // 이미 연결되지 않은 상태에서 다시 disconnect 시도
    TEST_ASSERT_EQUAL(LOGGER_STATUS_OK, LOGGER_Disconnect());
    
    // 다시 연결 (다음 테스트를 위해)
    LOGGER_Platform_Connect_ExpectAndReturn("127.0.0.1", 1234, LOGGER_STATUS_OK);
    LOGGER_Connect("127.0.0.1", 1234);
}

void test_LOGGER_Disconnect_should_handle_platform_error(void)
{
    // 플랫폼 disconnect가 실패하는 경우
    LOGGER_Platform_Disconnect_ExpectAndReturn(LOGGER_STATUS_ERROR);
    TEST_ASSERT_EQUAL(LOGGER_STATUS_ERROR, LOGGER_Disconnect());
    
    // 연결 상태는 그대로 유지되어야 함
    TEST_ASSERT_TRUE(LOGGER_IsConnected());
}

void test_LOGGER_Connect_should_handle_platform_error(void)
{
    // 연결 해제
    LOGGER_Platform_Disconnect_ExpectAndReturn(LOGGER_STATUS_OK);
    LOGGER_Disconnect();
    
    // 플랫폼 connect가 실패하는 경우
    LOGGER_Platform_Connect_ExpectAndReturn("192.168.1.1", 8080, LOGGER_STATUS_ERROR);
    TEST_ASSERT_EQUAL(LOGGER_STATUS_ERROR, LOGGER_Connect("192.168.1.1", 8080));
    
    // 연결 상태는 false여야 함
    TEST_ASSERT_FALSE(LOGGER_IsConnected());
    
    // 다시 연결 (다음 테스트를 위해)
    LOGGER_Platform_Connect_ExpectAndReturn("127.0.0.1", 1234, LOGGER_STATUS_OK);
    LOGGER_Connect("127.0.0.1", 1234);
}

void test_LOGGER_Connect_should_handle_null_server_ip(void)
{
    TEST_ASSERT_EQUAL(LOGGER_STATUS_ERROR, LOGGER_Connect(NULL, 1234));
}

#endif // TEST 