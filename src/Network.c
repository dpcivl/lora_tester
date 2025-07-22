#include "Network.h"
#include <string.h>
#include <stddef.h>

static bool g_connected = false;
static char g_server_ip[16] = {0};
static uint16_t g_server_port = 0;

int Network_Init(const char* server_ip, uint16_t port)
{
    if (server_ip == NULL || port == 0) {
        return NETWORK_INVALID_PARAM;
    }
    
    // IP 주소와 포트 저장
    strncpy(g_server_ip, server_ip, sizeof(g_server_ip) - 1);
    g_server_ip[sizeof(g_server_ip) - 1] = '\0';
    g_server_port = port;
    
    // 실제 구현에서는 소켓 생성 및 연결
    // 지금은 테스트를 위한 최소 구현
    g_connected = true;
    
    return NETWORK_OK;
}

int Network_SendBinary(const void* data, size_t size)
{
    if (!g_connected) {
        return NETWORK_NOT_CONNECTED;
    }
    
    if (data == NULL || size == 0) {
        return NETWORK_INVALID_PARAM;
    }
    
    // 실제 구현에서는 소켓을 통한 데이터 전송
    // 지금은 테스트를 위한 최소 구현
    
    return NETWORK_OK;
}

bool Network_IsConnected(void)
{
    return g_connected;
}

void Network_Disconnect(void)
{
    if (g_connected) {
        // 실제 구현에서는 소켓 해제
        g_connected = false;
        memset(g_server_ip, 0, sizeof(g_server_ip));
        g_server_port = 0;
    }
}