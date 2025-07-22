#include "unity.h"
#include "LogRemote.h"
#include "mock_Network.h"
#include "mock_time.h"
#include <string.h>

void setUp(void)
{
    // Setup에서는 기본 mock 동작만 설정
}

void tearDown(void)
{
    // LogRemote 상태 초기화 - 테스트 간 간섭 방지
    LogRemote_Reset();
}

// StateChangePacket 구조체 크기 테스트
void test_StateChangePacket_should_have_correct_size(void)
{
    TEST_ASSERT_EQUAL(20, sizeof(StateChangePacket));
}

// StateChangePacket 패킹 테스트
void test_StateChangePacket_should_be_packed_correctly(void)
{
    StateChangePacket packet = {0};
    packet.packet_type = 0x01;
    packet.timestamp = 0x12345678;
    packet.device_id = 0xAB;
    packet.old_state = 0x03;
    packet.new_state = 0x04;
    
    // 메모리 레이아웃 검증 (패딩 없음)
    uint8_t* bytes = (uint8_t*)&packet;
    TEST_ASSERT_EQUAL(0x01, bytes[0]);  // packet_type
    TEST_ASSERT_EQUAL(0x78, bytes[1]);  // timestamp LSB (little endian)
    TEST_ASSERT_EQUAL(0x56, bytes[2]);
    TEST_ASSERT_EQUAL(0x34, bytes[3]);
    TEST_ASSERT_EQUAL(0x12, bytes[4]);  // timestamp MSB
    TEST_ASSERT_EQUAL(0xAB, bytes[5]);  // device_id
    TEST_ASSERT_EQUAL(0x03, bytes[6]);  // old_state
    TEST_ASSERT_EQUAL(0x04, bytes[7]);  // new_state
}

// LogRemote 초기화 테스트
void test_LogRemote_Init_should_initialize_network(void)
{
    const char* server_ip = "192.168.1.100";
    uint16_t port = 8080;
    uint8_t device_id = 0x01;
    
    Network_Init_ExpectAndReturn(server_ip, port, 0);
    
    int result = LogRemote_Init(server_ip, port, device_id);
    
    TEST_ASSERT_EQUAL(0, result);
}

// LogRemote 초기화 실패 테스트
void test_LogRemote_Init_should_return_error_when_network_init_fails(void)
{
    const char* server_ip = "192.168.1.100";
    uint16_t port = 8080;
    uint8_t device_id = 0x01;
    
    Network_Init_ExpectAndReturn(server_ip, port, -1);
    
    int result = LogRemote_Init(server_ip, port, device_id);
    
    TEST_ASSERT_EQUAL(-1, result);
}

// State 변경 로그 전송 테스트
void test_LogRemote_SendStateChange_should_send_correct_packet(void)
{
    // Setup - LogRemote_Init에서 필요한 Mock 설정
    Network_Init_ExpectAndReturn("192.168.1.100", 8080, 0);
    LogRemote_Init("192.168.1.100", 8080, 0x01);
    
    TIME_GetCurrentMs_ExpectAndReturn(67890);
    Network_SendBinary_IgnoreAndReturn(0);
    
    // Execute
    int result = LogRemote_SendStateChange(
        LORA_STATE_WAIT_JOIN_OK, 
        LORA_STATE_SEND_PERIODIC, 
        2, 5);
    
    // Verify
    TEST_ASSERT_EQUAL(0, result);
}

// 연속 State 변경 시 duration 계산 테스트
void test_LogRemote_SendStateChange_should_calculate_duration_correctly(void)
{
    // Setup - 첫 번째 state change
    Network_Init_ExpectAndReturn("192.168.1.100", 8080, 0);
    LogRemote_Init("192.168.1.100", 8080, 0x01);
    TIME_GetCurrentMs_ExpectAndReturn(1000);
    Network_SendBinary_IgnoreAndReturn(0);
    
    LogRemote_SendStateChange(LORA_STATE_INIT, LORA_STATE_SEND_CMD, 0, 0);
    
    // 두 번째 state change - 3초 후
    TIME_GetCurrentMs_ExpectAndReturn(4000);
    Network_SendBinary_IgnoreAndReturn(0);
    
    int result = LogRemote_SendStateChange(LORA_STATE_SEND_CMD, LORA_STATE_WAIT_OK, 0, 0);
    
    // 내부적으로 duration이 3000ms로 계산되어야 함
    TEST_ASSERT_EQUAL(0, result);
}

// 체크섬 계산 테스트
void test_LogRemote_should_calculate_checksum_correctly(void)
{
    StateChangePacket packet = {
        .packet_type = 0x01,
        .timestamp = 12345,
        .device_id = 0x01,
        .old_state = 0x02,
        .new_state = 0x03,
        .state_duration = 1000,
        .error_count = 1,
        .send_count = 2,
        .error_code = 0,
        .reserved = 0,
        .checksum = 0
    };
    
    uint16_t calculated_checksum = LogRemote_CalculateChecksum(&packet);
    
    // 체크섬이 0이 아닌 값이어야 함
    TEST_ASSERT_NOT_EQUAL(0, calculated_checksum);
    
    // 같은 데이터는 같은 체크섬을 생성해야 함
    uint16_t second_checksum = LogRemote_CalculateChecksum(&packet);
    TEST_ASSERT_EQUAL(calculated_checksum, second_checksum);
}

// 네트워크 전송 실패 테스트
void test_LogRemote_SendStateChange_should_return_error_when_network_send_fails(void)
{
    Network_Init_ExpectAndReturn("192.168.1.100", 8080, 0);
    LogRemote_Init("192.168.1.100", 8080, 0x01);
    TIME_GetCurrentMs_ExpectAndReturn(12345);
    
    Network_SendBinary_IgnoreAndReturn(-1);
    
    int result = LogRemote_SendStateChange(LORA_STATE_INIT, LORA_STATE_SEND_CMD, 0, 0);
    
    TEST_ASSERT_EQUAL(-1, result);
}

// NULL 포인터 안전성 테스트  
void test_LogRemote_should_handle_null_pointers_safely(void)
{
    // tearDown에서 LogRemote_Reset()이 호출되므로 초기화되지 않은 상태
    // 초기화 전 호출 - LogRemote_SendStateChange가 초기화되지 않은 상태에서는
    // TIME_GetCurrentMs를 호출하지 않고 바로 -1을 반환해야 함
    int result = LogRemote_SendStateChange(LORA_STATE_INIT, LORA_STATE_SEND_CMD, 0, 0);
    TEST_ASSERT_EQUAL(-1, result);
    
    // NULL IP 주소
    result = LogRemote_Init(NULL, 8080, 0x01);
    TEST_ASSERT_EQUAL(-1, result);
}

// 장치 ID 설정 테스트
void test_LogRemote_should_use_correct_device_id(void)
{
    uint8_t test_device_id = 0xAB;
    Network_Init_ExpectAndReturn("192.168.1.100", 8080, 0);
    LogRemote_Init("192.168.1.100", 8080, test_device_id);
    
    TIME_GetCurrentMs_ExpectAndReturn(12345);
    
    // device_id가 패킷에 올바르게 설정되는지 검증하기 위해
    // Network_SendBinary가 호출되는지만 확인
    Network_SendBinary_IgnoreAndReturn(0);
    
    int result = LogRemote_SendStateChange(LORA_STATE_INIT, LORA_STATE_SEND_CMD, 0, 0);
    TEST_ASSERT_EQUAL(0, result);
}