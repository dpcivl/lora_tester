#include "Network.h"
#include "SDStorage.h"
#include <string.h>
#include <stddef.h>

static bool g_connected = false;
static char g_server_ip[16] = {0};
static uint16_t g_server_port = 0;
static NetworkBackend_t g_backend = NETWORK_BACKEND_SOCKET;

int Network_SetBackend(NetworkBackend_t backend)
{
    g_backend = backend;
    return NETWORK_OK;
}

NetworkBackend_t Network_GetBackend(void)
{
    return g_backend;
}

int Network_InitSD(void)
{
    if (g_backend != NETWORK_BACKEND_SD_CARD) {
        return NETWORK_ERROR;
    }
    
    int result = SDStorage_Init();
    if (result == SDSTORAGE_OK) {
        g_connected = true;
        return NETWORK_OK;
    }
    
    return NETWORK_ERROR;
}

int Network_Init(const char* server_ip, uint16_t port)
{
    if (g_backend != NETWORK_BACKEND_SOCKET) {
        return NETWORK_ERROR;
    }
    
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
    
    // 백엔드에 따른 분기 처리
    switch (g_backend) {
        case NETWORK_BACKEND_SOCKET:
            // 실제 구현에서는 소켓을 통한 데이터 전송
            // 지금은 테스트를 위한 최소 구현
            return NETWORK_OK;
            
        case NETWORK_BACKEND_SD_CARD:
            {
                int result = SDStorage_WriteLog(data, size);
                switch (result) {
                    case SDSTORAGE_OK:
                        return NETWORK_OK;
                    case SDSTORAGE_NOT_READY:
                        return NETWORK_NOT_CONNECTED;
                    case SDSTORAGE_INVALID_PARAM:
                        return NETWORK_INVALID_PARAM;
                    case SDSTORAGE_DISK_FULL:
                    case SDSTORAGE_FILE_ERROR:
                    default:
                        return NETWORK_ERROR;
                }
            }
            
        default:
            return NETWORK_ERROR;
    }
}

bool Network_IsConnected(void)
{
    if (g_backend == NETWORK_BACKEND_SD_CARD) {
        return g_connected && SDStorage_IsReady();
    }
    return g_connected;
}

void Network_Disconnect(void)
{
    if (g_connected) {
        // 백엔드별 해제 처리
        switch (g_backend) {
            case NETWORK_BACKEND_SOCKET:
                // 실제 구현에서는 소켓 해제
                memset(g_server_ip, 0, sizeof(g_server_ip));
                g_server_port = 0;
                break;
                
            case NETWORK_BACKEND_SD_CARD:
                SDStorage_Disconnect();
                break;
        }
        
        g_connected = false;
    }
}