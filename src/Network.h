#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 플랫폼 독립적 네트워크 인터페이스
// STM32: Ethernet, ESP32: WiFi, PC: Socket
// SD Storage: Local SD card logging

// 백엔드 타입 정의
typedef enum {
    NETWORK_BACKEND_SOCKET,    // 네트워크 소켓 통신 (기본값)
    NETWORK_BACKEND_SD_CARD    // SD카드 로컬 저장
} NetworkBackend_t;

// 백엔드 선택 설정
int Network_SetBackend(NetworkBackend_t backend);

// 현재 백엔드 확인
NetworkBackend_t Network_GetBackend(void);

// 네트워크 초기화 (소켓 백엔드용)
int Network_Init(const char* server_ip, uint16_t port);

// SD카드 백엔드 초기화
int Network_InitSD(void);

// 바이너리 데이터 전송
int Network_SendBinary(const void* data, size_t size);

// 연결 상태 확인
bool Network_IsConnected(void);

// 네트워크 해제
void Network_Disconnect(void);

// 에러 코드 정의
#define NETWORK_OK              0
#define NETWORK_ERROR          -1
#define NETWORK_NOT_CONNECTED  -2
#define NETWORK_TIMEOUT        -3
#define NETWORK_INVALID_PARAM  -4

#endif // NETWORK_H