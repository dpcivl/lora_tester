#include "LogRemote.h"
#include "Network.h"
#include "time.h"
#include <string.h>
#include <stddef.h>

static bool g_initialized = false;
static uint8_t g_device_id = 0;
static uint32_t g_last_state_time = 0;

int LogRemote_Init(const char* server_ip, uint16_t port, uint8_t device_id)
{
    if (server_ip == NULL) {
        return -1;
    }
    
    int result = Network_Init(server_ip, port);
    if (result == 0) {
        g_initialized = true;
        g_device_id = device_id;
        g_last_state_time = 0;
    }
    
    return result;
}

int LogRemote_SendStateChange(LoraState old_state, LoraState new_state, 
                             uint16_t error_count, uint16_t send_count)
{
    if (!g_initialized) {
        return -1;
    }
    
    uint32_t current_time = TIME_GetCurrentMs();
    uint32_t duration = 0;
    
    if (g_last_state_time > 0) {
        duration = current_time - g_last_state_time;
    }
    
    StateChangePacket packet = {
        .packet_type = PACKET_TYPE_STATE_CHANGE,
        .timestamp = current_time,
        .device_id = g_device_id,
        .old_state = (uint8_t)old_state,
        .new_state = (uint8_t)new_state,
        .state_duration = duration,
        .error_count = error_count,
        .send_count = send_count,
        .error_code = 0,
        .reserved = 0,
        .checksum = 0
    };
    
    // 체크섬 계산
    packet.checksum = LogRemote_CalculateChecksum(&packet);
    
    g_last_state_time = current_time;
    
    return Network_SendBinary(&packet, sizeof(packet));
}

uint16_t LogRemote_CalculateChecksum(const StateChangePacket* packet)
{
    if (packet == NULL) {
        return 0;
    }
    
    // 간단한 XOR 체크섬 (실제로는 CRC16 사용 권장)
    uint16_t checksum = 0;
    const uint8_t* data = (const uint8_t*)packet;
    
    // checksum 필드를 제외한 모든 바이트에 대해 계산
    for (size_t i = 0; i < sizeof(StateChangePacket) - sizeof(packet->checksum); i++) {
        checksum ^= data[i];
        checksum = (checksum << 1) | (checksum >> 15); // 좌 회전
    }
    
    return checksum;
}

void LogRemote_Disconnect(void)
{
    if (g_initialized) {
        Network_Disconnect();
        g_initialized = false;
        g_device_id = 0;
        g_last_state_time = 0;
    }
}

bool LogRemote_IsConnected(void)
{
    return g_initialized && Network_IsConnected();
}

void LogRemote_Reset(void)
{
    g_initialized = false;
    g_device_id = 0;
    g_last_state_time = 0;
}