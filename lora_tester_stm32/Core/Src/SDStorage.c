#include "SDStorage.h"
#include "logger.h"
#include <string.h>
#include <stdio.h>

// 플랫폼별 BSP 헤더 (FatFs 미들웨어와 통합)
#ifdef STM32F746xx
#include "stm32f7xx_hal.h"
#include "bsp_driver_sd.h"  // BSP SD 드라이버
extern UART_HandleTypeDef huart6;
extern RTC_HandleTypeDef hrtc;
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
    // STM32 환경: BSP 기반 FatFs 통합 초기화
    LOG_INFO("[SDStorage] Starting BSP-based SD card initialization...");
    
    // 1. BSP SD 드라이버 초기화 (FatFs와 완전 통합)
    uint8_t bsp_result = BSP_SD_Init();
    LOG_INFO("[SDStorage] BSP_SD_Init result: %d (0=OK, 1=ERROR, 2=NOT_PRESENT)", bsp_result);
    
    if (bsp_result != MSD_OK) {
        if (bsp_result == MSD_ERROR_SD_NOT_PRESENT) {
            LOG_ERROR("[SDStorage] SD card not detected - check physical connection");
        } else {
            LOG_ERROR("[SDStorage] BSP SD initialization failed");
        }
        return SDSTORAGE_ERROR;
    }
    
    // 2. BSP SD 카드 상태 확인
    uint8_t card_state = BSP_SD_GetCardState();
    LOG_INFO("[SDStorage] BSP SD card state: %d (0=TRANSFER_OK, 1=TRANSFER_BUSY)", card_state);
    
    if (card_state != SD_TRANSFER_OK) {
        LOG_WARN("[SDStorage] SD card not ready for transfer - state: %d", card_state);
        // 잠시 대기 후 재확인
        HAL_Delay(100);
        card_state = BSP_SD_GetCardState();
        LOG_INFO("[SDStorage] SD card state after delay: %d", card_state);
    }
    
    // 3. FatFs disk_initialize (BSP와 완전 통합됨)
    DSTATUS disk_status = disk_initialize(0);
    LOG_INFO("[SDStorage] FatFs disk_initialize result: 0x%02X (0x00=OK)", disk_status);
    
    if (disk_status != 0) {
        LOG_ERROR("[SDStorage] FatFs disk initialization failed - code: 0x%02X", disk_status);
        return SDSTORAGE_ERROR;
    }
    
    // 4. 파일시스템 마운트 (즉시 마운트로 변경 - BSP 안정성 확보됨)
    LOG_INFO("[SDStorage] Mounting file system with BSP integration...");
    FRESULT mount_result = f_mount(&SDFatFS, SDPath, 1);  // 즉시 마운트
    LOG_INFO("[SDStorage] f_mount result: %d (0=FR_OK)", mount_result);
    
    if (mount_result != FR_OK) {
        LOG_WARN("[SDStorage] Mount failed, attempting file system creation...");
        
        // 파일시스템 자동 생성 시도
        if (mount_result == FR_NOT_READY || mount_result == FR_NO_FILESYSTEM) {
            static BYTE work[_MAX_SS];
            
            LOG_INFO("[SDStorage] Creating file system...");
            FRESULT mkfs_result = f_mkfs(SDPath, FM_ANY, 0, work, sizeof(work));
            LOG_INFO("[SDStorage] f_mkfs result: %d", mkfs_result);
            
            if (mkfs_result != FR_OK) {
                LOG_ERROR("[SDStorage] File system creation failed: %d", mkfs_result);
                return SDSTORAGE_ERROR;
            }
            
            // 재마운트
            mount_result = f_mount(&SDFatFS, SDPath, 1);
            LOG_INFO("[SDStorage] Re-mount after mkfs: %d", mount_result);
            
            if (mount_result != FR_OK) {
                LOG_ERROR("[SDStorage] Re-mount failed: %d", mount_result);
                return SDSTORAGE_ERROR;
            }
        } else {
            LOG_ERROR("[SDStorage] Mount failed with error: %d", mount_result);
            return SDSTORAGE_ERROR;
        }
    }
    
    LOG_INFO("[SDStorage] BSP-FatFs integration successful");
#else
    // PC/테스트 환경: 시뮬레이션
    LOG_INFO("[SDStorage] Test environment - simulating BSP initialization");
#endif

    // 로그 디렉토리 생성
    LOG_INFO("[SDStorage] Creating log directory...");
    int dir_result = _create_log_directory();
    if (dir_result != SDSTORAGE_OK) {
        LOG_ERROR("[SDStorage] Failed to create log directory");
        LOG_WARN("[SDStorage] Continuing with terminal-only logging");
        g_sd_ready = false;
        return SDSTORAGE_ERROR;
    }
    
    g_sd_ready = true;
    g_current_log_size = 0;
    memset(g_current_log_file, 0, sizeof(g_current_log_file));
    
    LOG_INFO("[SDStorage] BSP-based SD storage initialization completed successfully");
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
    
    // BSP 기반 파일 생성
#ifdef STM32F746xx
    LOG_INFO("[SDStorage] Creating log file with BSP integration: %s", g_current_log_file);
    
    FRESULT open_result = f_open(&g_log_file, g_current_log_file, FA_CREATE_NEW | FA_WRITE);
    LOG_INFO("[SDStorage] f_open result: %d (0=FR_OK)", open_result);
    
    if (open_result != FR_OK) {
        if (open_result == FR_EXIST) {
            LOG_WARN("[SDStorage] File already exists - trying with different timestamp");
            // 타임스탬프 재생성으로 재시도
            HAL_Delay(1000);  // 1초 대기로 다른 타임스탬프 확보
            if (_generate_log_filename(g_current_log_file, sizeof(g_current_log_file)) == SDSTORAGE_OK) {
                open_result = f_open(&g_log_file, g_current_log_file, FA_CREATE_NEW | FA_WRITE);
                if (open_result == FR_OK) {
                    LOG_INFO("[SDStorage] File created with new timestamp");
                } else {
                    LOG_ERROR("[SDStorage] File creation failed even with new timestamp: %d", open_result);
                    g_sd_ready = false;
                    return SDSTORAGE_FILE_ERROR;
                }
            }
        } else {
            LOG_ERROR("[SDStorage] File creation failed: %d", open_result);
            g_sd_ready = false;
            return SDSTORAGE_FILE_ERROR;
        }
    }
    
    // 파일 생성 성공 - 닫기
    f_close(&g_log_file);
    g_file_open = false;
    LOG_INFO("[SDStorage] BSP-based log file created successfully");
#else
    // PC/테스트 환경: 시뮬레이션
    LOG_INFO("[SDStorage] Test environment - BSP file creation simulated");
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
    // STM32: BSP 기반 SD 카드 테스트
    LOG_INFO("[SDStorage] Testing SD card functionality with BSP drivers...");
    
    // 1. BSP SD 카드 읽기 테스트
    static uint32_t read_buffer[128];  // 512 bytes = 128 uint32_t
    uint8_t read_result = BSP_SD_ReadBlocks(read_buffer, 0, 1, 5000);
    LOG_INFO("[SDStorage] BSP_SD_ReadBlocks result: %d (0=OK)", read_result);
    
    if (read_result != MSD_OK) {
        LOG_ERROR("[SDStorage] BSP SD read test failed - hardware problem");
        return SDSTORAGE_ERROR;
    }
    
    // 2. BSP SD 카드 쓰기 테스트 (안전한 섹터 사용)
    static uint32_t write_buffer[128];
    memset(write_buffer, 0xAA, 512);  // 테스트 패턴
    
    LOG_INFO("[SDStorage] Testing BSP SD write capability...");
    uint8_t write_result = BSP_SD_WriteBlocks(write_buffer, 1000, 1, 5000);
    LOG_INFO("[SDStorage] BSP_SD_WriteBlocks result: %d (0=OK)", write_result);
    
    if (write_result != MSD_OK) {
        LOG_ERROR("[SDStorage] BSP SD write test failed - card may be write-protected");
        return SDSTORAGE_ERROR;
    }
    
    // 3. 쓰기 완료 대기 및 상태 확인
    LOG_INFO("[SDStorage] Waiting for write completion...");
    HAL_Delay(100);  // 쓰기 안정화 대기
    
    uint8_t card_state = BSP_SD_GetCardState();
    LOG_INFO("[SDStorage] Card state after write: %d (0=TRANSFER_OK)", card_state);
    
    // 4. 쓰기 검증
    static uint32_t verify_buffer[128];
    uint8_t verify_result = BSP_SD_ReadBlocks(verify_buffer, 1000, 1, 5000);
    LOG_INFO("[SDStorage] BSP_SD_ReadBlocks(verify) result: %d", verify_result);
    
    bool verification_ok = false;
    if (verify_result == MSD_OK) {
        int mismatch_count = 0;
        for (int i = 0; i < 128; i++) {
            if (write_buffer[i] != verify_buffer[i]) {
                mismatch_count++;
            }
        }
        
        if (mismatch_count == 0) {
            LOG_INFO("[SDStorage] ✅ BSP SD read/write test successful");
            verification_ok = true;
        } else {
            LOG_WARN("[SDStorage] ⚠️ BSP SD data verification failed - %d mismatches", mismatch_count);
            verification_ok = false;
        }
    } else {
        LOG_ERROR("[SDStorage] BSP SD verify read failed");
        verification_ok = false;
    }
    
    // 5. 하드웨어 테스트 결과 평가
    if (write_result != MSD_OK || verify_result != MSD_OK || !verification_ok) {
        LOG_WARN("[SDStorage] BSP hardware test failed - attempting simple file test");
    } else {
        LOG_INFO("[SDStorage] BSP hardware test passed - proceeding with directory creation");
    }
    
    // 6. FatFs 레벨 테스트 - 로그 디렉토리 생성
    LOG_INFO("[SDStorage] Creating log directory: %s", SDSTORAGE_LOG_DIR);
    FRESULT mkdir_result = f_mkdir(SDSTORAGE_LOG_DIR);
    LOG_INFO("[SDStorage] f_mkdir result: %d (0=OK, 8=FR_EXIST)", mkdir_result);
    
    if (mkdir_result != FR_OK && mkdir_result != FR_EXIST) {
        LOG_WARN("[SDStorage] Directory creation failed: %d, testing root directory access", mkdir_result);
        
        // 루트 디렉토리에서 테스트 파일 생성
        FIL test_file;
        FRESULT test_result = f_open(&test_file, "bsp_test.txt", FA_CREATE_NEW | FA_WRITE);
        
        if (test_result == FR_OK) {
            const char* test_data = "BSP Test\n";
            UINT bytes_written;
            f_write(&test_file, test_data, strlen(test_data), &bytes_written);
            f_close(&test_file);
            f_unlink("bsp_test.txt");  // 정리
            
            LOG_INFO("[SDStorage] ✅ Root directory file test successful");
            return SDSTORAGE_OK;  // 루트에서 파일 생성 가능
        } else {
            LOG_ERROR("[SDStorage] Root directory file test failed: %d", test_result);
            return SDSTORAGE_ERROR;
        }
    }
    
    // 7. 디렉토리 생성 성공 - 추가 테스트
    FIL test_file;
    char test_path[64];
    snprintf(test_path, sizeof(test_path), "%s/bsp_test.txt", SDSTORAGE_LOG_DIR);
    
    FRESULT file_test = f_open(&test_file, test_path, FA_CREATE_NEW | FA_WRITE);
    if (file_test == FR_OK) {
        const char* test_data = "BSP Directory Test\n";
        UINT bytes_written;
        f_write(&test_file, test_data, strlen(test_data), &bytes_written);
        f_close(&test_file);
        f_unlink(test_path);  // 정리
        
        LOG_INFO("[SDStorage] ✅ Directory and file creation fully functional");
        return SDSTORAGE_OK;
    } else {
        LOG_ERROR("[SDStorage] Directory file test failed: %d", file_test);
        return SDSTORAGE_ERROR;
    }
    
#else
    // PC: 시뮬레이션
    LOG_INFO("[SDStorage] Test environment - BSP directory creation simulated");
    return SDSTORAGE_OK;
#endif
}

static int _generate_log_filename(char* filename, size_t max_len)
{
    // 타임스탬프는 향후 확장용으로 유지
    uint32_t timestamp = _get_current_timestamp();
    (void)timestamp;  // 경고 방지용
    
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
    
    // 디렉토리 포함하여 파일 생성 (클럭 최적화 후 정상 동작 기대)
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