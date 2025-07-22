#ifndef LOGREMOTE_H
#define LOGREMOTE_H

#include <stdint.h>
#include <stdbool.h>
#include "LoraStarter.h"

// 원격 로그 패킷 구조체 (20바이트 고정)
#pragma pack(push, 1)
typedef struct {
    uint8_t packet_type;        // 1byte: 패킷 타입 (0x01: State Change)
    uint32_t timestamp;         // 4byte: Unix timestamp (ms)
    uint8_t device_id;          // 1byte: 장치 ID
    uint8_t old_state;          // 1byte: 이전 상태
    uint8_t new_state;          // 1byte: 현재 상태
    uint32_t state_duration;    // 4byte: 이전 상태 유지 시간 (ms)
    uint16_t error_count;       // 2byte: 에러 카운터
    uint16_t send_count;        // 2byte: 송신 카운터
    uint8_t error_code;         // 1byte: 에러 코드 (필요시)
    uint8_t reserved;           // 1byte: 패딩
    uint16_t checksum;          // 2byte: CRC16 체크섬
} StateChangePacket;          // 총 20바이트
#pragma pack(pop)

// LogRemote 모듈 함수들
int LogRemote_Init(const char* server_ip, uint16_t port, uint8_t device_id);
int LogRemote_SendStateChange(LoraState old_state, LoraState new_state, 
                             uint16_t error_count, uint16_t send_count);
uint16_t LogRemote_CalculateChecksum(const StateChangePacket* packet);
void LogRemote_Disconnect(void);
bool LogRemote_IsConnected(void);
void LogRemote_Reset(void);  // 테스트용 리셋 함수

// 패킷 타입 정의
#define PACKET_TYPE_STATE_CHANGE    0x01
#define PACKET_TYPE_ERROR_LOG       0x02
#define PACKET_TYPE_HEARTBEAT       0x03

#endif // LOGREMOTE_H