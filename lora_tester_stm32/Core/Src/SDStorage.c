#include "SDStorage.h"
#include "logger.h"
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
// 파일 닫기 보장을 위한 전역 추적 시스템
static FIL* g_current_file_handle = NULL;  // 현재 열린 파일 핸들 추적
static char g_current_open_file[256] = {0};  // 현재 열린 파일명 추적
#else
static FILE* g_log_file = NULL;
#endif

// 파일 닫기 보장 함수들
#ifdef STM32F746xx
static void _ensure_file_closed(void) {
    if (g_current_file_handle != NULL) {
        LOG_DEBUG("[SDStorage] Force closing previously opened file: %s", g_current_open_file);
        f_close(g_current_file_handle);
        g_current_file_handle = NULL;
        memset(g_current_open_file, 0, sizeof(g_current_open_file));
    }
}

static void _register_file_opened(FIL* file_handle, const char* filename) {
    g_current_file_handle = file_handle;
    strncpy(g_current_open_file, filename, sizeof(g_current_open_file) - 1);
    g_current_open_file[sizeof(g_current_open_file) - 1] = '\0';
}

static void _register_file_closed(void) {
    g_current_file_handle = NULL;
    memset(g_current_open_file, 0, sizeof(g_current_open_file));
}
#endif

// 내부 함수 선언
static int _create_log_directory(void);
static int _generate_log_filename(char* filename, size_t max_len);
// static uint32_t _get_current_timestamp(void); - unused function removed

int SDStorage_Init(void)
{
#ifdef STM32F746xx
    // STM32 환경: FatFs 초기화 및 진단
    LOG_INFO("[SDStorage] Starting SD card initialization...");
    
    // 초기화 시 파일 닫기 보장
    _ensure_file_closed();
    
    // 1. 하드웨어 상태 진단 및 TRANSFER 상태까지 대기
    extern SD_HandleTypeDef hsd1;
    HAL_SD_CardStateTypeDef card_state = HAL_SD_GetCardState(&hsd1);
    LOG_INFO("[SDStorage] Initial SD card state: %d", card_state);
    
    // SD 카드가 TRANSFER 상태가 될 때까지 대기 (성공 프로젝트 패턴)
    int wait_count = 0;
    while (card_state != HAL_SD_CARD_TRANSFER && wait_count < 50) {  // 최대 5초 대기
        LOG_INFO("[SDStorage] Waiting for SD card TRANSFER state... (attempt %d)", wait_count + 1);
        HAL_Delay(100);
        card_state = HAL_SD_GetCardState(&hsd1);
        wait_count++;
    }
    
    if (card_state == HAL_SD_CARD_TRANSFER) {
        LOG_INFO("[SDStorage] ✅ SD card reached TRANSFER state successfully");
        
        // SDMMC 에러 코드 상세 체크 (성공 프로젝트 패턴)
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
    } else {
        LOG_ERROR("[SDStorage] ❌ SD card failed to reach TRANSFER state (state: %d)", card_state);
        LOG_ERROR("[SDStorage] SDMMC ErrorCode: 0x%08X", hsd1.ErrorCode);
        return SDSTORAGE_ERROR;
    }
    
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
    
    // f_mount 호출 전에 충분한 지연 (SD 카드 안정화)
    #ifdef STM32F746xx
    LOG_INFO("[SDStorage] Waiting for SD card stabilization (500ms)...");
    HAL_Delay(500);
    #endif
    
    // f_mount 블로킹 문제 - 완전 우회 시도
    LOG_WARN("[SDStorage] f_mount consistently blocks despite all fixes");
    LOG_INFO("[SDStorage] Attempting direct file operations without f_mount...");
    LOG_INFO("[SDStorage] Some FatFs implementations support auto-mount on first file access");
    
    // f_mount 여러 번 재시도 (성공 프로젝트 패턴)
    LOG_INFO("[SDStorage] Attempting f_mount with retry logic...");
    FRESULT mount_result = FR_DISK_ERR;  // 초기값
    
    for (int retry = 0; retry < 3; retry++) {
        LOG_INFO("[SDStorage] f_mount attempt %d/3...", retry + 1);
        mount_result = f_mount(&SDFatFS, SDPath, 1);  // 즉시 마운트
        LOG_INFO("[SDStorage] f_mount result: %d", mount_result);
        
        if (mount_result == FR_OK) {
            LOG_INFO("[SDStorage] ✅ f_mount successful on attempt %d", retry + 1);
            break;
        } else {
            LOG_WARN("[SDStorage] f_mount failed on attempt %d, retrying in 1000ms...", retry + 1);
            if (retry < 2) {  // 마지막 시도가 아니면 대기
                // STM32F7 D-Cache 클리어 (성공 프로젝트 패턴)
                LOG_INFO("[SDStorage] Clearing D-Cache for STM32F7 compatibility...");
                SCB_CleanInvalidateDCache();
                HAL_Delay(1000);
            }
        }
    }
    
    // 즉시 마운트 성공 시 쓰기 준비 완료
    if (mount_result == FR_OK) {
        LOG_INFO("[SDStorage] Immediate mount successful - SD ready for write operations");
    }
    
    if (mount_result != FR_OK) {
        LOG_WARN("[SDStorage] f_mount failed with result: %d", mount_result);
        
        // SD 카드가 이미 포맷되어 있다면 f_mkfs 시도하지 않고 다른 접근법 사용
        if (mount_result == FR_DISK_ERR) {
            LOG_WARN("[SDStorage] FR_DISK_ERR detected - SD card may be formatted but incompatible");
            LOG_INFO("[SDStorage] Skipping f_mkfs since SD card is already FAT32 formatted");
            LOG_INFO("[SDStorage] Trying alternative mount approach...");
            
            // 다른 마운트 방식 시도 (지연 마운트)
            LOG_INFO("[SDStorage] Attempting deferred mount (flag=0)...");
            mount_result = f_mount(&SDFatFS, SDPath, 0);
            LOG_INFO("[SDStorage] Deferred mount result: %d", mount_result);
            
            if (mount_result == FR_OK) {
                LOG_INFO("[SDStorage] Deferred mount successful!");
            } else {
                LOG_ERROR("[SDStorage] Both immediate and deferred mount failed");
                LOG_ERROR("[SDStorage] SD card may have hardware compatibility issues");
                return SDSTORAGE_ERROR;
            }
        }
        else if (mount_result == FR_NOT_READY || mount_result == FR_NO_FILESYSTEM) {
            // 작업 버퍼 할당 (전역 또는 스택)
            static BYTE work[_MAX_SS];
            
            // 실제 f_mkfs 시도
            LOG_INFO("[SDStorage] Attempting to create filesystem with f_mkfs...");
            FRESULT mkfs_result = f_mkfs(SDPath, FM_ANY, 0, work, sizeof(work));
            LOG_INFO("[SDStorage] f_mkfs(FM_ANY) result: %d", mkfs_result);
            
            if (mkfs_result != FR_OK) {
                // FAT32로 다시 시도
                LOG_INFO("[SDStorage] Retrying with explicit FAT32 format...");
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

    // FatFs 마운트 성공 확인됨
    
    // 디렉토리 생성 시도
    LOG_INFO("[SDStorage] Creating log directory...");
    int dir_result = _create_log_directory();
    g_directory_available = (dir_result == SDSTORAGE_OK);
    
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
    // STM32 환경: 안정적인 열기-쓰기-닫기 방식
    
    // 로그 파일명이 없으면 생성
    if (strlen(g_current_log_file) == 0) {
        if (_generate_log_filename(g_current_log_file, sizeof(g_current_log_file)) != SDSTORAGE_OK) {
            LOG_ERROR("[SDStorage] Failed to generate log filename");
            return SDSTORAGE_ERROR;
        }
    }
    
    // SD 카드 상태 변화에 robust한 방식: 매번 열고 닫기
    FIL temp_file;
    memset(&temp_file, 0, sizeof(temp_file));
    
    // 파일 닫기 보장: 이전에 열린 파일이 있으면 강제로 닫기
    _ensure_file_closed();
    
    // 성공 프로젝트 방식: 디스크 상태 먼저 확인
    DSTATUS current_disk_stat = disk_status(0);
    if (current_disk_stat != 0) {
        LOG_WARN("[SDStorage] Disk not ready (%d), reinitializing...", current_disk_stat);
        DSTATUS init_result = disk_initialize(0);
        if (init_result != 0) {
            LOG_ERROR("[SDStorage] Disk reinitialization failed: %d", init_result);
            return SDSTORAGE_NOT_READY;
        }
    }
    
    // 파일 열기 (성공 프로젝트 방식: 단계적 시도)
    FRESULT open_result = f_open(&temp_file, g_current_log_file, FA_OPEN_APPEND | FA_WRITE);
    
    // 파일 열기 성공 시 추적 등록
    if (open_result == FR_OK) {
        _register_file_opened(&temp_file, g_current_log_file);
        LOG_DEBUG("[SDStorage] File opened and registered: %s", g_current_log_file);
    }
    
    // f_open 실패 시 성공 프로젝트 방식의 복구 로직
    if (open_result != FR_OK) {
        LOG_WARN("[SDStorage] f_open failed (%d), trying recovery...", open_result);
        
        // 1단계: 마운트 재시도
        f_mount(NULL, SDPath, 0);  // 언마운트
        HAL_Delay(200);
        FRESULT remount_result = f_mount(&SDFatFS, SDPath, 1);  // 강제 재마운트
        
        if (remount_result == FR_OK) {
            // 재마운트 성공 후 다시 파일 열기 시도
            open_result = f_open(&temp_file, g_current_log_file, FA_OPEN_APPEND | FA_WRITE);
            if (open_result == FR_OK) {
                _register_file_opened(&temp_file, g_current_log_file);
                LOG_INFO("[SDStorage] File opened after remount recovery");
            }
        }
        
        // 2단계: 여전히 실패하면 f_mkfs 시도
        if (open_result != FR_OK) {
            LOG_WARN("[SDStorage] File still failed, trying f_mkfs recovery...");
            static BYTE work[4096];
            FRESULT mkfs_result = f_mkfs(SDPath, FM_ANY, 0, work, sizeof(work));
            
            if (mkfs_result == FR_OK) {
                LOG_INFO("[SDStorage] f_mkfs successful, remounting...");
                f_mount(NULL, SDPath, 0);
                HAL_Delay(500);
                remount_result = f_mount(&SDFatFS, SDPath, 1);
                
                if (remount_result == FR_OK) {
                    // 파일명 재생성 (mkfs 후 파일이 사라졌으므로)
                    _generate_log_filename(g_current_log_file, sizeof(g_current_log_file));
                    open_result = f_open(&temp_file, g_current_log_file, FA_CREATE_ALWAYS | FA_WRITE);
                    if (open_result == FR_OK) {
                        _register_file_opened(&temp_file, g_current_log_file);
                        LOG_INFO("[SDStorage] File created after f_mkfs recovery");
                    }
                }
            }
        }
        
        // 모든 복구 시도 실패
        if (open_result != FR_OK) {
            LOG_ERROR("[SDStorage] All recovery attempts failed: %d", open_result);
            return SDSTORAGE_FILE_ERROR;
        }
    }
    
    if (open_result == FR_OK) {
        // FA_OPEN_APPEND 사용 시 자동으로 파일 끝에 위치
        
        // 원본 데이터 쓰기
        UINT bytes_written;
        FRESULT write_result = f_write(&temp_file, data, size, &bytes_written);
        
        if (write_result == FR_OK && bytes_written == size) {
            // 줄바꿈 추가 (Windows 호환을 위해 \r\n 사용)
            UINT newline_written;
            FRESULT newline_result = f_write(&temp_file, "\r\n", 2, &newline_written);
            
            // 즉시 동기화 및 닫기
            f_sync(&temp_file);
            f_close(&temp_file);
            _register_file_closed();  // 추적 해제
            
            if (newline_result == FR_OK) {
                g_current_log_size += bytes_written + newline_written;
                LOG_DEBUG("[SDStorage] Log written successfully: %d+2 bytes", bytes_written);
                return SDSTORAGE_OK;
            } else {
                LOG_WARN("[SDStorage] Newline write failed: %d", newline_result);
                g_current_log_size += bytes_written;  // 원본 데이터는 성공했으므로 카운트
                return SDSTORAGE_OK;  // 원본 데이터 쓰기는 성공했으므로 OK 반환
            }
        } else {
            f_close(&temp_file);  // 실패해도 파일 닫기
            _register_file_closed();  // 추적 해제
            if (write_result != FR_OK) {
                LOG_ERROR("[SDStorage] f_write failed: %d", write_result);
                return SDSTORAGE_FILE_ERROR;
            } else if (bytes_written != size) {
                LOG_WARN("[SDStorage] Partial write: %d/%d bytes", bytes_written, size);
                return SDSTORAGE_DISK_FULL;
            }
        }
    } else {
        LOG_ERROR("[SDStorage] f_open failed: %d - SD card state may have changed", open_result);
        
        // SD 카드 상태 재확인
        DSTATUS current_status = disk_status(0);
        LOG_WARN("[SDStorage] Current disk status: 0x%02X", current_status);
        
        if (current_status != 0) {
            LOG_WARN("[SDStorage] SD card not ready - temporarily disabling SD logging");
            return SDSTORAGE_NOT_READY;
        }
        
        return SDSTORAGE_FILE_ERROR;
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
        // 파일 닫기 보장 후 마운트 해제
        _ensure_file_closed();
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

// static uint32_t _get_current_timestamp(void) - unused function removed
// {
// #ifdef STM32F746xx
//     return HAL_GetTick();
// #else
//     return (uint32_t)time(NULL);
// #endif
// }