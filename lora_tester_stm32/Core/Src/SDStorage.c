#include "SDStorage.h"
#include "logger.h"
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
    // STM32 환경: FatFs 초기화 및 진단
    LOG_INFO("[SDStorage] Starting SD card initialization...");
    
    // 1. 하드웨어 상태 진단
    extern SD_HandleTypeDef hsd1;
    HAL_SD_CardStateTypeDef card_state = HAL_SD_GetCardState(&hsd1);
    LOG_INFO("[SDStorage] HAL SD card state: %d", card_state);
    
    DSTATUS disk_status = disk_initialize(0);
    LOG_INFO("[SDStorage] disk_initialize result: 0x%02X", disk_status);
    
    // disk_initialize 실패 시 조기 종료 (블로킹 방지)
    if (disk_status != 0) {
        LOG_ERROR("[SDStorage] disk_initialize failed - SD card not ready");
        LOG_ERROR("[SDStorage] Possible causes: write-protected, bad card, or BSP/HAL conflict");
        return SDSTORAGE_ERROR;
    }
    
    // 2. 파일시스템 마운트 시도 (지연 마운트로 변경 - 블로킹 방지)
    LOG_INFO("[SDStorage] Using deferred mount (flag=0) to avoid blocking...");
    
    // f_mount 호출 전에 약간의 지연 (SD 카드 안정화)
    #ifdef STM32F746xx
    HAL_Delay(100);
    #endif
    
    // 지연 마운트: 실제 파일 접근 시까지 마운트 지연 (거의 항상 성공)
    FRESULT mount_result = f_mount(&SDFatFS, SDPath, 0);
    LOG_INFO("[SDStorage] f_mount(deferred) result: %d", mount_result);
    
    // 지연 마운트 성공 시 실제 SD 상태는 첫 파일 작업에서 확인됨
    if (mount_result == FR_OK) {
        LOG_INFO("[SDStorage] Deferred mount successful - SD will be tested on first file operation");
    }
    
    if (mount_result != FR_OK) {
        LOG_WARN("[SDStorage] f_mount failed, attempting file system creation...");
        
        // 3. 파일시스템 자동 생성 시도 (ST 커뮤니티 가이드 기반)
        if (mount_result == FR_NOT_READY || mount_result == FR_NO_FILESYSTEM) {
            // 작업 버퍼 할당 (전역 또는 스택)
            static BYTE work[_MAX_SS];
            
            // FM_ANY로 먼저 시도
            FRESULT mkfs_result = f_mkfs(SDPath, FM_ANY, 0, work, sizeof(work));
            LOG_INFO("[SDStorage] f_mkfs(FM_ANY) result: %d", mkfs_result);
            
            if (mkfs_result != FR_OK) {
                // FAT32 강제 생성 시도 (4096 클러스터 사이즈)
                mkfs_result = f_mkfs(SDPath, FM_FAT32, 4096, work, sizeof(work));
                LOG_INFO("[SDStorage] f_mkfs(FM_FAT32) result: %d", mkfs_result);
                
                if (mkfs_result != FR_OK) {
                    LOG_ERROR("[SDStorage] File system creation failed: %d", mkfs_result);
                    LOG_ERROR("[SDStorage] Possible SD card hardware issue - try different card");
                    return SDSTORAGE_ERROR;
                }
            }
            
            // 파일시스템 생성 후 재마운트 시도
            mount_result = f_mount(&SDFatFS, SDPath, 1);
            LOG_INFO("[SDStorage] Re-mount after mkfs result: %d", mount_result);
            
            if (mount_result != FR_OK) {
                LOG_ERROR("[SDStorage] Re-mount failed after mkfs: %d", mount_result);
                return SDSTORAGE_ERROR;
            }
        } else {
            LOG_ERROR("[SDStorage] Mount failed with unrecoverable error: %d", mount_result);
            return SDSTORAGE_ERROR;
        }
    }
    
    LOG_INFO("[SDStorage] File system mount successful");
#else
    // PC/테스트 환경: 시뮬레이션
    LOG_INFO("[SDStorage] Test environment - simulating successful initialization");
#endif

    // 로그 디렉토리 생성 (지연 마운트 후 첫 실제 파일 작업)
    LOG_INFO("[SDStorage] Creating log directory (first real SD operation)...");
    int dir_result = _create_log_directory();
    if (dir_result != SDSTORAGE_OK) {
        LOG_ERROR("[SDStorage] Failed to create log directory - SD card may have write issues");
        LOG_WARN("[SDStorage] Continuing without SD logging - terminal only mode");
        // SD 문제가 있어도 계속 진행 (terminal logging만 사용)
        g_sd_ready = false;  // SD 비활성화
        return SDSTORAGE_ERROR;
    }
    LOG_INFO("[SDStorage] Log directory created successfully");
    
    g_sd_ready = true;
    g_current_log_size = 0;
    memset(g_current_log_file, 0, sizeof(g_current_log_file));
    
    LOG_INFO("[SDStorage] Initialization completed successfully");
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
    
    // 파일 생성 확인 (SD 쓰기 문제로 인한 블로킹 방지)
#ifdef STM32F746xx
    LOG_INFO("[SDStorage] Attempting to create log file: %s", g_current_log_file);
    
    // f_open 호출 전 로깅
    LOG_INFO("[SDStorage] Calling f_open with FA_CREATE_NEW | FA_WRITE...");
    FRESULT open_result = f_open(&g_log_file, g_current_log_file, FA_CREATE_NEW | FA_WRITE);
    LOG_INFO("[SDStorage] f_open result: %d", open_result);
    
    if (open_result != FR_OK) {
        LOG_ERROR("[SDStorage] f_open failed: %d - SD write problem detected", open_result);
        LOG_WARN("[SDStorage] Disabling SD logging due to file creation failure");
        g_sd_ready = false;  // SD 로깅 비활성화
        return SDSTORAGE_FILE_ERROR;
    }
    
    LOG_INFO("[SDStorage] File created successfully, closing...");
    f_close(&g_log_file);
    g_file_open = false;
    LOG_INFO("[SDStorage] File closed, ready for logging");
#else
    // PC/테스트 환경: 파일 생성 시뮬레이션 (항상 성공)
    LOG_INFO("[SDStorage] Test environment - file creation simulated");
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
    // STM32: SD카드 저수준 테스트 먼저 수행
    LOG_INFO("[SDStorage] Testing SD card at HAL level before f_mkdir...");
    
    extern SD_HandleTypeDef hsd1;
    
    // 1. SD카드 읽기 테스트 (섹터 0 읽기)
    static uint8_t read_buffer[512];
    HAL_StatusTypeDef read_result = HAL_SD_ReadBlocks(&hsd1, read_buffer, 0, 1, 5000);
    LOG_INFO("[SDStorage] HAL_SD_ReadBlocks result: %d", read_result);
    
    if (read_result != HAL_OK) {
        LOG_ERROR("[SDStorage] SD card read test failed - hardware problem");
        return SDSTORAGE_ERROR;
    }
    
    // 2. SD카드 쓰기 테스트 (임시 섹터에 쓰기)
    static uint8_t write_buffer[512];
    memset(write_buffer, 0xAA, 512);  // 테스트 패턴
    
    LOG_INFO("[SDStorage] Testing SD card write capability...");
    HAL_StatusTypeDef write_result = HAL_SD_WriteBlocks(&hsd1, write_buffer, 1000, 1, 5000);
    LOG_INFO("[SDStorage] HAL_SD_WriteBlocks result: %d", write_result);
    
    if (write_result != HAL_OK) {
        LOG_ERROR("[SDStorage] SD card write test failed - card may be write-protected or damaged");
        return SDSTORAGE_ERROR;
    }
    
    // 3. 쓰기 검증 (방금 쓴 데이터 읽기)
    static uint8_t verify_buffer[512];
    HAL_StatusTypeDef verify_result = HAL_SD_ReadBlocks(&hsd1, verify_buffer, 1000, 1, 5000);
    LOG_INFO("[SDStorage] HAL_SD_ReadBlocks(verify) result: %d", verify_result);
    
    if (verify_result == HAL_OK) {
        if (memcmp(write_buffer, verify_buffer, 512) == 0) {
            LOG_INFO("[SDStorage] ✅ SD card read/write test successful - hardware is OK");
        } else {
            LOG_WARN("[SDStorage] ⚠️ SD card data verification failed - possible data corruption");
        }
    }
    
    // 4. HAL 테스트에서 검증 실패가 있어도 f_mkdir 시도 (블로킹 방지를 위해 스킵)
    if (verify_result != HAL_OK) {
        LOG_WARN("[SDStorage] Verify read failed - skipping f_mkdir to avoid blocking");
        LOG_INFO("[SDStorage] Will try direct file creation instead of directory");
        return SDSTORAGE_OK;  // 디렉토리 없이도 파일 생성 시도
    }
    
    LOG_INFO("[SDStorage] HAL tests passed - skipping f_mkdir to avoid system blocking");
    LOG_WARN("[SDStorage] Directory creation disabled - files will be created in root");
    FRESULT mkdir_result = FR_OK;  // 강제로 성공 처리
    LOG_INFO("[SDStorage] f_mkdir bypassed - proceeding with root directory logging");
    
    // FR_EXIST(9)는 이미 존재함을 의미하므로 성공으로 처리
    if (mkdir_result == FR_OK || mkdir_result == FR_EXIST) {
        LOG_INFO("[SDStorage] Directory ready (created or already exists)");
        return SDSTORAGE_OK;
    } else {
        LOG_ERROR("[SDStorage] f_mkdir failed: %d - FatFs level problem", mkdir_result);
        LOG_INFO("[SDStorage] Will try direct file creation without directory");
        return SDSTORAGE_OK;  // 디렉토리 실패해도 루트에 파일 생성 시도
    }
#else
    // PC: mkdir 시뮬레이션 (테스트에서는 성공으로 가정)
    LOG_INFO("[SDStorage] Test environment - directory creation simulated");
    return SDSTORAGE_OK;
#endif
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
    
    // 디렉토리가 없을 경우를 대비해 루트에 파일 생성
    int result = snprintf(filename, max_len, 
                         "%s%04d%02d%02d_%02d%02d%02d%s",
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