#ifdef TEST

#include "unity.h"
#include "mock_logger.h"
#include <stdbool.h>

#include "ResponseHandler.h"

void setUp(void)
{
    LOGGER_SendWithLevel_IgnoreAndReturn(LOGGER_STATUS_OK);
}

void tearDown(void)
{
}


bool is_response_ok(const char* response);
bool is_join_response_ok(const char* response);

void test_is_response_ok_should_return_true_for_OK(void)
{
    TEST_ASSERT_TRUE(is_response_ok("OK"));
}

void test_is_response_ok_should_return_false_for_ERROR(void)
{
    TEST_ASSERT_FALSE(is_response_ok("ERROR"));
}

void test_is_response_ok_should_return_false_for_empty_string(void)
{
    TEST_ASSERT_FALSE(is_response_ok(""));
}

void test_is_response_ok_should_return_true_for_OK_with_newline(void)
{
    TEST_ASSERT_TRUE(is_response_ok("OK\r\n"));
}

void test_ResponseHandler_ParseSendResponse_should_return_TIMEOUT_for_TIMEOUT(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_TIMEOUT, ResponseHandler_ParseSendResponse("TIMEOUT"));
}

void test_ResponseHandler_ParseSendResponse_should_return_UNKNOWN_for_unknown_response(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_UNKNOWN, ResponseHandler_ParseSendResponse("UNKNOWN"));
}

void test_ResponseHandler_ParseSendResponse_should_return_UNKNOWN_for_NULL(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_UNKNOWN, ResponseHandler_ParseSendResponse(NULL));
}

void test_ResponseHandler_ParseSendResponse_should_return_UNKNOWN_for_empty_string(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_UNKNOWN, ResponseHandler_ParseSendResponse(""));
}

void test_ResponseHandler_ParseSendResponse_should_return_OK_for_SEND_CONFIRMED_OK(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_OK, ResponseHandler_ParseSendResponse("+EVT:SEND_CONFIRMED_OK"));
}

void test_ResponseHandler_ParseSendResponse_should_return_ERROR_for_SEND_CONFIRMED_FAILED(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_ERROR, ResponseHandler_ParseSendResponse("+EVT:SEND_CONFIRMED_FAILED(4)"));
}

void test_ResponseHandler_ParseSendResponse_should_return_ERROR_for_SEND_CONFIRMED_FAILED_without_number(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_ERROR, ResponseHandler_ParseSendResponse("+EVT:SEND_CONFIRMED_FAILED"));
}

void test_is_join_response_ok_should_return_true_for_JOINED(void)
{
    TEST_ASSERT_TRUE(is_join_response_ok("+EVT:JOINED"));
}

void test_is_join_response_ok_should_return_false_for_other_responses(void)
{
    TEST_ASSERT_FALSE(is_join_response_ok("OK"));
    TEST_ASSERT_FALSE(is_join_response_ok("ERROR"));
    TEST_ASSERT_FALSE(is_join_response_ok(""));
}

void test_is_response_ok_should_return_false_for_NULL(void)
{
    TEST_ASSERT_FALSE(is_response_ok(NULL));
}

void test_is_response_ok_should_return_false_for_partial_match(void)
{
    TEST_ASSERT_FALSE(is_response_ok("OK_EXTRA"));
    TEST_ASSERT_FALSE(is_response_ok("PREFIX_OK"));
    TEST_ASSERT_FALSE(is_response_ok("OK_SUFFIX"));
}

void test_is_response_ok_should_return_false_for_lowercase(void)
{
    TEST_ASSERT_FALSE(is_response_ok("ok"));
    TEST_ASSERT_FALSE(is_response_ok("Ok"));
    TEST_ASSERT_FALSE(is_response_ok("oK"));
}

void test_is_response_ok_should_return_false_for_whitespace(void)
{
    TEST_ASSERT_FALSE(is_response_ok(" OK"));
    TEST_ASSERT_FALSE(is_response_ok("OK "));
    TEST_ASSERT_FALSE(is_response_ok(" OK "));
}

void test_is_join_response_ok_should_return_false_for_NULL(void)
{
    TEST_ASSERT_FALSE(is_join_response_ok(NULL));
}

void test_is_join_response_ok_should_return_false_for_partial_match(void)
{
    TEST_ASSERT_FALSE(is_join_response_ok("+EVT:JOINED_EXTRA"));
    TEST_ASSERT_FALSE(is_join_response_ok("PREFIX_+EVT:JOINED"));
    TEST_ASSERT_FALSE(is_join_response_ok("+EVT:JOINED_SUFFIX"));
}

void test_is_join_response_ok_should_return_false_for_lowercase(void)
{
    TEST_ASSERT_FALSE(is_join_response_ok("+evt:joined"));
    TEST_ASSERT_FALSE(is_join_response_ok("+EVT:joined"));
    TEST_ASSERT_FALSE(is_join_response_ok("+evt:JOINED"));
}

void test_ResponseHandler_ParseSendResponse_should_return_UNKNOWN_for_unexpected_format(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_UNKNOWN, ResponseHandler_ParseSendResponse("+EVT:UNKNOWN_EVENT"));
    TEST_ASSERT_EQUAL(RESPONSE_UNKNOWN, ResponseHandler_ParseSendResponse("SOME_OTHER_RESPONSE"));
    TEST_ASSERT_EQUAL(RESPONSE_UNKNOWN, ResponseHandler_ParseSendResponse("+EVT:DIFFERENT_EVENT"));
}

void test_ResponseHandler_ParseSendResponse_should_return_ERROR_for_SEND_CONFIRMED_FAILED_with_different_number(void)
{
    TEST_ASSERT_EQUAL(RESPONSE_ERROR, ResponseHandler_ParseSendResponse("+EVT:SEND_CONFIRMED_FAILED(1)"));
    TEST_ASSERT_EQUAL(RESPONSE_ERROR, ResponseHandler_ParseSendResponse("+EVT:SEND_CONFIRMED_FAILED(10)"));
    TEST_ASSERT_EQUAL(RESPONSE_ERROR, ResponseHandler_ParseSendResponse("+EVT:SEND_CONFIRMED_FAILED(999)"));
}

#endif // TEST
