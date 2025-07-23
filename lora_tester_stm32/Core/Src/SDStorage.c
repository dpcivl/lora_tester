#include "SDStorage.h"
#include <string.h>
#include <stdio.h>

// 플랫폼별 HAL 헤더
#ifdef STM32F746xx
#include "stm32f7xx_hal.h"
extern UART_HandleTypeDef huart6;
#endif

// 플랫폼별 조건부 컴파일
#ifdef STM32F746xx
#include "fatfs.h"
#include "main.h"  // RTC handle과 타입 정의
#else
// PC/테스트 환경에서는 파일 I/O 시뮬레이션
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#endif

// 내부 상태 관리
static bool g_sd_ready = false;
static char g_current_log_file[256] = {0};
static size_t g_current_log_size = 0;

#ifdef STM32F746xx
static FIL g_log_file;
static bool g_file_open = false;
#else
static FILE* g_log_file = NULL;
#endif

// 내부 함수 선언
static int _create_log_directory(void);
static int _generate_log_filename(char* filename, size_t max_len);
static uint32_t _get_current_timestamp(void);

int SDStorage_Init(void)
{
#ifdef STM32F746xx
    // STM32 환경: FatFs 초기화
    FRESULT mount_result = f_mount(&SDFatFS, SDPath, 1);
    if (mount_result != FR_OK) {
        // 터미널로 직접 출력 (Logger 사용 시 무한루프 방지)
        char error_msg[100];
        sprintf(error_msg, "[SDStorage] f_mount failed: %d\r\n", mount_result);
        HAL_UART_Transmit(&huart6, (uint8_t*)error_msg, strlen(error_msg), 1000);
        return SDSTORAGE_ERROR;
    }
    
    // SD 카드 상태 확인
    FATFS *fs;
    DWORD fre_clust;
    FRESULT free_result = f_getfree(SDPath, &fre_clust, &fs);
    if (free_result != FR_OK) {
        char error_msg[100];
        sprintf(error_msg, "[SDStorage] f_getfree failed: %d\r\n", free_result);
        HAL_UART_Transmit(&huart6, (uint8_t*)error_msg, strlen(error_msg), 1000);
        return SDSTORAGE_ERROR;
    }
    
    char success_msg[100];
    sprintf(success_msg, "[SDStorage] SD Card OK, Free: %lu KB\r\n", fre_clust * fs->csize / 2);
    HAL_UART_Transmit(&huart6, (uint8_t*)success_msg, strlen(success_msg), 1000);
#else
    // PC/테스트 환경: 시뮬레이션
    // 실제로는 파일 시스템이 준비되었다고 가정
#endif

    // 로그 디렉토리 생성
    if (_create_log_directory() != SDSTORAGE_OK) {
        return SDSTORAGE_ERROR;
    }
    
    g_sd_ready = true;
    g_current_log_size = 0;
    memset(g_current_log_file, 0, sizeof(g_current_log_file));
    
    return SDSTORAGE_OK;
}

int SDStorage_WriteLog(const void* data, size_t size)
{
    if (!g_sd_ready) {
        return SDSTORAGE_NOT_READY;
    }
    
    if (data == NULL || size == 0) {
        return SDSTORAGE_INVALID_PARAM;
    }
    
    // 새 로그 파일이 필요한 경우 생성
    if (strlen(g_current_log_file) == 0 || 
        g_current_log_size + size > SDSTORAGE_MAX_LOG_SIZE) {
        if (SDStorage_CreateNewLogFile() != SDSTORAGE_OK) {
            return SDSTORAGE_FILE_ERROR;
        }
    }

#ifdef STM32F746xx
    // STM32 환경: FatFs를 사용한 파일 쓰기
    if (!g_file_open) {
        if (f_open(&g_log_file, g_current_log_file, FA_WRITE | FA_OPEN_APPEND) != FR_OK) {
            return SDSTORAGE_FILE_ERROR;
        }
        g_file_open = true;
    }
    
    UINT bytes_written;
    if (f_write(&g_log_file, data, size, &bytes_written) != FR_OK) {
        return SDSTORAGE_FILE_ERROR;
    }
    
    if (bytes_written != size) {
        return SDSTORAGE_DISK_FULL;
    }
    
    // 즉시 플러시하여 데이터 안정성 확보
    f_sync(&g_log_file);
#else
    // PC/테스트 환경: 파일 I/O 시뮬레이션 (항상 성공)
    // 실제 파일 쓰기 없이 성공으로 처리
#endif

    g_current_log_size += size;
    return SDSTORAGE_OK;
}

bool SDStorage_IsReady(void)
{
    return g_sd_ready;
}

void SDStorage_Disconnect(void)
{
    if (g_sd_ready) {
#ifdef STM32F746xx
        if (g_file_open) {
            f_close(&g_log_file);
            g_file_open = false;
        }
        f_mount(NULL, SDPath, 0);
#else
        if (g_log_file != NULL) {
            fclose(g_log_file);
            g_log_file = NULL;
        }
#endif
        
        g_sd_ready = false;
        g_current_log_size = 0;
        memset(g_current_log_file, 0, sizeof(g_current_log_file));
    }
}

int SDStorage_CreateNewLogFile(void)
{
    if (!g_sd_ready) {
        return SDSTORAGE_NOT_READY;
    }
    
    // 이전 파일이 열려있다면 닫기
#ifdef STM32F746xx
    if (g_file_open) {
        f_close(&g_log_file);
        g_file_open = false;
    }
#else
    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
#endif
    
    // 새 파일명 생성
    if (_generate_log_filename(g_current_log_file, sizeof(g_current_log_file)) != SDSTORAGE_OK) {
        return SDSTORAGE_ERROR;
    }
    
    // 파일 생성 확인
#ifdef STM32F746xx
    if (f_open(&g_log_file, g_current_log_file, FA_CREATE_NEW | FA_WRITE) != FR_OK) {
        return SDSTORAGE_FILE_ERROR;
    }
    f_close(&g_log_file);
    g_file_open = false;
#else
    // PC/테스트 환경: 파일 생성 시뮬레이션 (항상 성공)
    // 실제 파일 생성 없이 성공으로 처리
#endif
    
    g_current_log_size = 0;
    return SDSTORAGE_OK;
}

size_t SDStorage_GetCurrentLogSize(void)
{
    return g_current_log_size;
}

// 내부 함수 구현
static int _create_log_directory(void)
{
#ifdef STM32F746xx
    // STM32: FatFs 디렉토리 생성
    f_mkdir(SDSTORAGE_LOG_DIR);  // 이미 존재해도 에러 무시
#else
    // PC: mkdir 시뮬레이션 (테스트에서는 성공으로 가정)
    // 실제로는 디렉토리 생성 시뮬레이션
#endif
    return SDSTORAGE_OK;
}

static int _generate_log_filename(char* filename, size_t max_len)
{
    uint32_t timestamp = _get_current_timestamp();
    
    // YYYYMMDD_HHMMSS 형식으로 타임스탬프 생성
    uint16_t year = 2025;   // 기본값
    uint8_t month = 1, day = 1, hour = 0, minute = 0, second = 0;
    
#ifdef STM32F746xx
    // STM32: RTC에서 실제 시간 읽기
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK) {
        year = 2000 + sDate.Year;
        month = sDate.Month;
        day = sDate.Date;
        hour = sTime.Hours;
        minute = sTime.Minutes;
        second = sTime.Seconds;
    }
#else
    // PC: 간단한 타임스탬프 시뮬레이션 (테스트용)
    // 실제 시간 대신 고정된 값 사용
    year = 2025;
    month = 7;
    day = 23;
    hour = 10;
    minute = 30;
    second = timestamp % 60;  // 타임스탬프 기반 변화
#endif
    
    int result = snprintf(filename, max_len, 
                         "%s/%s%04d%02d%02d_%02d%02d%02d%s",
                         SDSTORAGE_LOG_DIR,
                         SDSTORAGE_LOG_PREFIX,
                         year, month, day, hour, minute, second,
                         SDSTORAGE_LOG_EXTENSION);
    
    if (result < 0 || (size_t)result >= max_len) {
        return SDSTORAGE_ERROR;
    }
    
    return SDSTORAGE_OK;
}

static uint32_t _get_current_timestamp(void)
{
#ifdef STM32F746xx
    return HAL_GetTick();
#else
    return (uint32_t)time(NULL);
#endif
}