#include "unity.h"
#include "Network.h"
#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

// 네트워크 초기화 테스트
void test_Network_Init_should_return_success_with_valid_params(void)
{
    const char* server_ip = "192.168.1.100";
    uint16_t port = 8080;
    
    int result = Network_Init(server_ip, port);
    
    // 실제 구현에서는 성공 시 0 반환
    TEST_ASSERT_EQUAL(0, result);
}

// 잘못된 IP 주소 테스트
void test_Network_Init_should_return_error_with_null_ip(void)
{
    int result = Network_Init(NULL, 8080);
    
    TEST_ASSERT_EQUAL(-1, result);
}

// 잘못된 포트 테스트
void test_Network_Init_should_return_error_with_invalid_port(void)
{
    int result = Network_Init("192.168.1.100", 0);
    
    TEST_ASSERT_EQUAL(-1, result);
}

// 바이너리 데이터 전송 테스트
void test_Network_SendBinary_should_send_data_successfully(void)
{
    // 초기화 필요
    Network_Init("192.168.1.100", 8080);
    
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t data_size = sizeof(test_data);
    
    int result = Network_SendBinary(test_data, data_size);
    
    TEST_ASSERT_EQUAL(0, result);
}

// 초기화 없이 전송 시도 테스트
void test_Network_SendBinary_should_return_error_when_not_initialized(void)
{
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    int result = Network_SendBinary(test_data, sizeof(test_data));
    
    TEST_ASSERT_EQUAL(-1, result);
}

// NULL 데이터 전송 테스트
void test_Network_SendBinary_should_return_error_with_null_data(void)
{
    Network_Init("192.168.1.100", 8080);
    
    int result = Network_SendBinary(NULL, 10);
    
    TEST_ASSERT_EQUAL(-1, result);
}

// 0 크기 데이터 전송 테스트
void test_Network_SendBinary_should_return_error_with_zero_size(void)
{
    Network_Init("192.168.1.100", 8080);
    
    uint8_t test_data[] = {0x01, 0x02};
    int result = Network_SendBinary(test_data, 0);
    
    TEST_ASSERT_EQUAL(-1, result);
}

// 큰 데이터 전송 테스트
void test_Network_SendBinary_should_handle_large_data(void)
{
    Network_Init("192.168.1.100", 8080);
    
    uint8_t large_data[1024];
    memset(large_data, 0xAA, sizeof(large_data));
    
    int result = Network_SendBinary(large_data, sizeof(large_data));
    
    TEST_ASSERT_EQUAL(0, result);
}

// 연결 상태 확인 테스트
void test_Network_IsConnected_should_return_false_when_not_initialized(void)
{
    bool connected = Network_IsConnected();
    
    TEST_ASSERT_FALSE(connected);
}

// 연결 상태 확인 테스트 (초기화 후)
void test_Network_IsConnected_should_return_true_when_initialized(void)
{
    Network_Init("192.168.1.100", 8080);
    
    bool connected = Network_IsConnected();
    
    TEST_ASSERT_TRUE(connected);
}

// 네트워크 해제 테스트
void test_Network_Disconnect_should_disconnect_successfully(void)
{
    Network_Init("192.168.1.100", 8080);
    
    Network_Disconnect();
    
    // 해제 후 연결 상태는 false
    bool connected = Network_IsConnected();
    TEST_ASSERT_FALSE(connected);
}

// 재연결 테스트
void test_Network_should_support_reconnection(void)
{
    // 첫 번째 연결
    Network_Init("192.168.1.100", 8080);
    TEST_ASSERT_TRUE(Network_IsConnected());
    
    // 해제
    Network_Disconnect();
    TEST_ASSERT_FALSE(Network_IsConnected());
    
    // 재연결
    int result = Network_Init("192.168.1.101", 9090);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_TRUE(Network_IsConnected());
}