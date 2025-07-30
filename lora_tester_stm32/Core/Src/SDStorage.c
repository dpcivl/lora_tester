#include "SDStorage.h"
#include "logger.h"
#include "system_config.h"
#include <string.h>
#include <stdio.h>

// 플랫폼별 HAL 헤더
#ifdef STM32F746xx
#include "stm32f7xx_hal.h"
extern UART_HandleTypeDef huart6;
extern SD_HandleTypeDef hsd1;  // SD 핸들 선언 추가
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
static bool g_directory_available = false;  // 디렉토리 사용 가능 여부

#ifdef STM32F746xx
// 지속적 파일 핸들 시스템 (한 번 열어두고 계속 사용)
static FIL g_persistent_log_file;  // 지속적으로 열려있는 로그 파일
static bool g_file_is_open = false;  // 파일이 열려있는 상태 추적

#else
static FILE* g_log_file = NULL;
#endif

// 내부 함수 구현 - 함수 호출 순서에 맞게 배치
#ifdef STM32F746xx
static int _generate_log_filename(char* filename, size_t max_len)
{
    // 8.3 형식 파일명 생성 - 기존 파일 확인하여 중복 방지
    static int file_counter = 0;  // 0부터 시작하여 첫 번째 호출에서 1로 설정
    
    // 첫 번째 호출에서만 기존 파일 확인
    if (file_counter == 0) {
        file_counter = 1;
        
        // 기존 파일들 확인하여 다음 번호 찾기
        for (int i = 1; i <= 9999; i++) {
            char test_filename[256];
            FIL test_file;
            
            if (g_directory_available) {
                snprintf(test_filename, sizeof(test_filename), "lora_logs/LORA%04d.TXT", i);
            } else {
                snprintf(test_filename, sizeof(test_filename), "LORA%04d.TXT", i);
            }
            
            // 파일이 존재하는지 확인
            FRESULT test_result = f_open(&test_file, test_filename, FA_READ);
            if (test_result == FR_OK) {
                f_close(&test_file);
                file_counter = i + 1;  // 다음 번호로 설정
            } else {
                break;  // 파일이 없으면 현재 번호 사용
            }
        }
        
        LOG_DEBUG("[SDStorage] Auto-detected next log file number: %d", file_counter);
    }
    
    // 디렉토리 사용 가능 여부에 따라 경로 결정
    int result;
    if (g_directory_available) {
        // lora_logs 디렉토리에 파일 생성 (TXT 형식)
        result = snprintf(filename, max_len, "lora_logs/LORA%04d.TXT", file_counter);
    } else {
        // 루트 디렉토리에 파일 생성 (TXT 형식)
        result = snprintf(filename, max_len, "LORA%04d.TXT", file_counter);
    }
    
    file_counter++;
    
    if (result < 0 || (size_t)result >= max_len) {
        return SDSTORAGE_ERROR;
    }
    
    return SDSTORAGE_OK;
}

// 지속적 파일 핸들 관리 함수들
static void _ensure_persistent_file_open(void) {
    if (!g_file_is_open || strlen(g_current_log_file) == 0) {
        // 파일이 열려있지 않거나 파일명이 없으면 새로 열기
        if (g_file_is_open) {
            f_close(&g_persistent_log_file);
            g_file_is_open = false;
        }
        
        // 파일명 생성 (필요시)
        if (strlen(g_current_log_file) == 0) {
            _generate_log_filename(g_current_log_file, sizeof(g_current_log_file));
        }
        
        // 파일 열기 (append 모드)
        FRESULT open_result = f_open(&g_persistent_log_file, g_current_log_file, FA_OPEN_APPEND | FA_WRITE);
        if (open_result != FR_OK) {
            // 파일이 없으면 생성
            open_result = f_open(&g_persistent_log_file, g_current_log_file, FA_CREATE_ALWAYS | FA_WRITE);
        }
        
        if (open_result == FR_OK) {
            g_file_is_open = true;
            LOG_DEBUG("[SDStorage] Persistent file opened: %s", g_current_log_file);
        } else {
            LOG_ERROR("[SDStorage] Failed to open persistent file: %d", open_result);
        }
    }
}

static void _close_persistent_file(void) {
    if (g_file_is_open) {
        f_close(&g_persistent_log_file);
        g_file_is_open = false;
        LOG_DEBUG("[SDStorage] Persistent file closed: %s", g_current_log_file);
    }
}

#endif

// 내부 함수 선언
static int _create_log_directory(void);
static void _close_persistent_file(void);
static int _initialize_sd_hardware(void);
static int _mount_filesystem_with_retry(void);
// static uint32_t _get_current_timestamp(void); - unused function removed

int SDStorage_Init(void)
{
    LOG_INFO("[SDStorage] Starting SD card initialization...");
    
    // 초기화 시 지속적 파일 닫기
    _close_persistent_file();
    
    // 1. SD 하드웨어 초기화 및 상태 확인
    int hw_result = _initialize_sd_hardware();
    if (hw_result != SDSTORAGE_OK) {
        return hw_result;
    }
    
    // 2. 파일시스템 마운트 (재시도 로직 포함)
    int mount_result = _mount_filesystem_with_retry();
    if (mount_result != SDSTORAGE_OK) {
        return mount_result;
    }
    
    LOG_INFO("[SDStorage] File system mount successful");
    
    // 3. 디렉토리 생성 시도
    LOG_INFO("[SDStorage] Creating log directory...");
    int dir_result = _create_log_directory();
    g_directory_available = (dir_result == SDSTORAGE_OK);
    
    // 4. 최종 상태 설정
    g_sd_ready = true;
    
    // 기존 로그 파일명이 있으면 보존, 크기는 리셋하지 않음
    if (strlen(g_current_log_file) > 0) {
        LOG_INFO("[SDStorage] Preserving existing log file: %s (size: %d bytes)", 
                 g_current_log_file, g_current_log_size);
    } else {
        // 첫 초기화인 경우에만 크기와 파일명 초기화
        g_current_log_size = 0;
        memset(g_current_log_file, 0, sizeof(g_current_log_file));
        LOG_INFO("[SDStorage] First initialization - log file will be created on first write");
    }
    
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
    
    // 새 로그 파일이 필요한 경우 생성 (파일 크기 체크는 일단 생략)
    if (strlen(g_current_log_file) == 0) {
        if (SDStorage_CreateNewLogFile() != SDSTORAGE_OK) {
            return SDSTORAGE_FILE_ERROR;
        }
    }

#ifdef STM32F746xx
    // 새로운 방식: 지속적 파일 핸들 사용 (한 번 열어두고 계속 쓰기)
    _ensure_persistent_file_open();
    
    if (!g_file_is_open) {
        LOG_ERROR("[SDStorage] Cannot open persistent file");
        return SDSTORAGE_FILE_ERROR;
    }
    
    // 데이터 + 줄바꿈 추가하여 쓰기
    char write_buffer[LOGGER_WRITE_BUFFER_SIZE];
    if (size + 2 < sizeof(write_buffer)) {
        // 원본 데이터 복사
        memcpy(write_buffer, data, size);
        // 줄바꿈 추가
        write_buffer[size] = '\r';
        write_buffer[size + 1] = '\n';
        
        // 파일에 쓰기 (파일은 이미 열려있음)
        UINT bytes_written;
        FRESULT write_result = f_write(&g_persistent_log_file, write_buffer, size + 2, &bytes_written);
        
        if (write_result == FR_OK && bytes_written == size + 2) {
            // 즉시 동기화 (파일은 열린 상태로 유지)
            f_sync(&g_persistent_log_file);
            g_current_log_size += bytes_written;
            LOG_DEBUG("[SDStorage] Persistent write successful: %d bytes", bytes_written);
            return SDSTORAGE_OK;
        } else {
            LOG_ERROR("[SDStorage] Persistent write failed: %d, written: %d/%d", write_result, bytes_written, size + 2);
            // 쓰기 실패 시 파일 다시 열기 시도
            _close_persistent_file();
            return SDSTORAGE_FILE_ERROR;
        }
    } else {
        LOG_ERROR("[SDStorage] Data too large for write buffer: %d bytes", size);
        return SDSTORAGE_INVALID_PARAM;
    }
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
        // 지속적 파일 닫기 후 마운트 해제
        _close_persistent_file();
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
    
    // 전역 파일 객체 제거됨 - 별도 처리 불필요
    
    // 새 파일명 생성
    if (_generate_log_filename(g_current_log_file, sizeof(g_current_log_file)) != SDSTORAGE_OK) {
        return SDSTORAGE_ERROR;
    }
    
    // 파일 생성 테스트 (간단한 방식)
#ifdef STM32F746xx
    LOG_INFO("[SDStorage] Testing file creation: %s", g_current_log_file);
    
    // 지역 변수로 파일 객체 생성
    FIL test_file;
    memset(&test_file, 0, sizeof(test_file));
    
    // SD 카드 상태 재확인
    DSTATUS current_disk_status = disk_status(0);
    LOG_INFO("[SDStorage] Current disk status: 0x%02X", current_disk_status);
    
    // 파일 생성 테스트
    FRESULT open_result = f_open(&test_file, g_current_log_file, FA_CREATE_ALWAYS | FA_WRITE);
    LOG_INFO("[SDStorage] f_open result: %d", open_result);
    
    if (open_result != FR_OK) {
        LOG_ERROR("[SDStorage] f_open failed: %d", open_result);
        
        // 상세 에러 분석
        switch (open_result) {
            case 16: // FR_INVALID_OBJECT
                LOG_ERROR("[SDStorage] FR_INVALID_OBJECT - File object initialization issue");
                break;
            case 9: // FR_WRITE_PROTECTED  
                LOG_ERROR("[SDStorage] FR_WRITE_PROTECTED - SD card is write protected");
                break;
            case 3: // FR_NOT_READY
                LOG_ERROR("[SDStorage] FR_NOT_READY - Disk not ready");
                break;
            default:
                LOG_ERROR("[SDStorage] Unknown f_open error: %d", open_result);
                break;
        }
        
        LOG_WARN("[SDStorage] Disabling SD logging due to file creation failure");
        g_sd_ready = false;  // SD 로깅 비활성화
        return SDSTORAGE_FILE_ERROR;
    }
    
    // 파일 생성 확인 후 즉시 닫기 (추적 등록 없이)
    f_close(&test_file);
    LOG_INFO("[SDStorage] File created and ready for logging: %s", g_current_log_file);
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
    // FatFs가 이미 정상 동작하므로 HAL 테스트 불필요
    
    // f_mkdir 전에 볼륨 상태 재확인 (에러 6 방지)
    LOG_INFO("[SDStorage] Verifying volume state before f_mkdir...");
    
    // 볼륨 재마운트 시도 (상태 안정화)
    FRESULT remount_result = f_mount(&SDFatFS, SDPath, 1);
    LOG_INFO("[SDStorage] Volume re-mount result: %d", remount_result);
    
    FRESULT mkdir_result = FR_NOT_ENABLED;  // 초기값 설정
    
    if (remount_result == FR_OK) {
        LOG_INFO("[SDStorage] Volume ready - attempting f_mkdir...");
        mkdir_result = f_mkdir("lora_logs");
        LOG_INFO("[SDStorage] f_mkdir result: %d", mkdir_result);
    } else {
        LOG_ERROR("[SDStorage] Volume re-mount failed: %d", remount_result);
    }
    
    // FR_EXIST(9)는 이미 존재함을 의미하므로 성공으로 처리
    if (mkdir_result == FR_OK || mkdir_result == FR_EXIST) {
        LOG_INFO("[SDStorage] Directory ready (created or already exists)");
        return SDSTORAGE_OK;  // 디렉토리 성공
    } else {
        LOG_ERROR("[SDStorage] f_mkdir failed: %d - FatFs level problem", mkdir_result);
        LOG_INFO("[SDStorage] Will try direct file creation without directory");
        return SDSTORAGE_ERROR;  // 디렉토리 실패
    }
#else
    // PC: mkdir 시뮬레이션 (테스트에서는 성공으로 가정)
    LOG_INFO("[SDStorage] Test environment - directory creation simulated");
    return SDSTORAGE_OK;
#endif
}

// SD 하드웨어 초기화 및 상태 확인
static int _initialize_sd_hardware(void)
{
#ifdef STM32F746xx
    extern SD_HandleTypeDef hsd1;
    HAL_SD_CardStateTypeDef card_state = HAL_SD_GetCardState(&hsd1);
    LOG_INFO("[SDStorage] Initial SD card state: %d", card_state);
    
    // SD 카드가 TRANSFER 상태가 될 때까지 대기
    int wait_count = 0;
    while (card_state != HAL_SD_CARD_TRANSFER && wait_count < SD_TRANSFER_WAIT_MAX_COUNT) {
        LOG_INFO("[SDStorage] Waiting for SD card TRANSFER state... (attempt %d)", wait_count + 1);
        HAL_Delay(SD_TRANSFER_CHECK_INTERVAL_MS);
        card_state = HAL_SD_GetCardState(&hsd1);
        wait_count++;
    }
    
    if (card_state == HAL_SD_CARD_TRANSFER) {
        LOG_INFO("[SDStorage] ✅ SD card reached TRANSFER state successfully");
        
        // SDMMC 에러 코드 상세 체크
        if (hsd1.ErrorCode != HAL_SD_ERROR_NONE) {
            LOG_WARN("[SDStorage] SDMMC ErrorCode detected: 0x%08X", hsd1.ErrorCode);
            
            if (hsd1.ErrorCode & SDMMC_ERROR_TX_UNDERRUN) {
                LOG_WARN("[SDStorage] TX_UNDERRUN detected - clock may be too fast");
            }
            if (hsd1.ErrorCode & SDMMC_ERROR_DATA_CRC_FAIL) {
                LOG_WARN("[SDStorage] CRC_FAIL detected - cache issue possible");
                SCB_CleanInvalidateDCache();
            }
            
            // 에러 코드 클리어
            hsd1.ErrorCode = HAL_SD_ERROR_NONE;
        }
        
        // disk_initialize 호출
        DSTATUS disk_status = disk_initialize(0);
        LOG_INFO("[SDStorage] disk_initialize result: 0x%02X", disk_status);
        
        if (disk_status != 0) {
            LOG_ERROR("[SDStorage] disk_initialize failed - SD card not ready");
            LOG_ERROR("[SDStorage] Possible causes: write-protected, bad card, or BSP/HAL conflict");
            return SDSTORAGE_ERROR;
        }
        
        return SDSTORAGE_OK;
    } else {
        LOG_ERROR("[SDStorage] ❌ SD card failed to reach TRANSFER state (state: %d)", card_state);
        LOG_ERROR("[SDStorage] SDMMC ErrorCode: 0x%08X", hsd1.ErrorCode);
        return SDSTORAGE_ERROR;
    }
#else
    return SDSTORAGE_OK;  // PC 환경에서는 성공으로 처리
#endif
}

// 파일시스템 마운트 (재시도 로직 포함)
static int _mount_filesystem_with_retry(void)
{
#ifdef STM32F746xx
    // SD 카드 안정화 대기
    LOG_INFO("[SDStorage] Waiting for SD card stabilization (%dms)...", SD_CARD_STABILIZE_DELAY_MS);
    HAL_Delay(SD_CARD_STABILIZE_DELAY_MS);
    
    // f_mount 여러 번 재시도
    LOG_INFO("[SDStorage] Attempting f_mount with retry logic...");
    FRESULT mount_result = FR_DISK_ERR;
    
    for (int retry = 0; retry < SD_MOUNT_RETRY_COUNT; retry++) {
        LOG_INFO("[SDStorage] f_mount attempt %d/%d...", retry + 1, SD_MOUNT_RETRY_COUNT);
        mount_result = f_mount(&SDFatFS, SDPath, 1);  // 즉시 마운트
        LOG_INFO("[SDStorage] f_mount result: %d", mount_result);
        
        if (mount_result == FR_OK) {
            LOG_INFO("[SDStorage] ✅ f_mount successful on attempt %d", retry + 1);
            return SDSTORAGE_OK;
        } else {
            LOG_WARN("[SDStorage] f_mount failed on attempt %d, retrying in %dms...", retry + 1, SD_MOUNT_RETRY_DELAY_MS);
            if (retry < SD_MOUNT_RETRY_COUNT - 1) {  // 마지막 시도가 아니면 대기
                // STM32F7 D-Cache 클리어
                LOG_INFO("[SDStorage] Clearing D-Cache for STM32F7 compatibility...");
                SCB_CleanInvalidateDCache();
                HAL_Delay(SD_MOUNT_RETRY_DELAY_MS);
            }
        }
    }
    
    // 모든 재시도 실패 시 추가 복구 시도
    if (mount_result != FR_OK) {
        LOG_WARN("[SDStorage] f_mount failed with result: %d", mount_result);
        
        if (mount_result == FR_DISK_ERR) {
            LOG_WARN("[SDStorage] FR_DISK_ERR detected - trying deferred mount...");
            mount_result = f_mount(&SDFatFS, SDPath, 0);
            LOG_INFO("[SDStorage] Deferred mount result: %d", mount_result);
            
            if (mount_result == FR_OK) {
                LOG_INFO("[SDStorage] Deferred mount successful!");
                return SDSTORAGE_OK;
            }
        }
        else if (mount_result == FR_NOT_READY || mount_result == FR_NO_FILESYSTEM) {
            // 파일시스템 생성 시도
            static BYTE work[_MAX_SS];
            LOG_INFO("[SDStorage] Attempting to create filesystem with f_mkfs...");
            FRESULT mkfs_result = f_mkfs(SDPath, FM_ANY, 0, work, sizeof(work));
            LOG_INFO("[SDStorage] f_mkfs(FM_ANY) result: %d", mkfs_result);
            
            if (mkfs_result != FR_OK) {
                mkfs_result = f_mkfs(SDPath, FM_FAT32, 4096, work, sizeof(work));
                LOG_INFO("[SDStorage] f_mkfs(FM_FAT32) result: %d", mkfs_result);
            }
            
            if (mkfs_result == FR_OK) {
                // 파일시스템 생성 후 재마운트
                mount_result = f_mount(&SDFatFS, SDPath, 1);
                LOG_INFO("[SDStorage] Re-mount after mkfs result: %d", mount_result);
                
                if (mount_result == FR_OK) {
                    return SDSTORAGE_OK;
                }
            }
        }
        
        LOG_ERROR("[SDStorage] All mount attempts failed");
        return SDSTORAGE_ERROR;
    }
    
    return SDSTORAGE_OK;
#else
    return SDSTORAGE_OK;  // PC 환경에서는 성공으로 처리
#endif
}