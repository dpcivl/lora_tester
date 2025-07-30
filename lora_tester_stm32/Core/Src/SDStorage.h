#ifndef SDSTORAGE_H
#define SDSTORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "error_codes.h"

// SD카드 기반 로그 저장 모듈
// FatFs 라이브러리를 사용하여 LoRa 통신 로그를 SD카드에 저장

// SD Storage 초기화
ResultCode SDStorage_Init(void);

// 바이너리 데이터를 SD카드에 저장
ResultCode SDStorage_WriteLog(const void* data, size_t size);

// SD카드 준비 상태 확인
bool SDStorage_IsReady(void);

// SD카드 저장소 해제
void SDStorage_Disconnect(void);

// 새로운 로그 파일 생성 (타임스탬프 기반)
ResultCode SDStorage_CreateNewLogFile(void);

// 현재 로그 파일 크기 확인
size_t SDStorage_GetCurrentLogSize(void);

// 호환성을 위한 기존 에러 코드 매핑 (deprecated)
#define SDSTORAGE_OK              RESULT_SUCCESS
#define SDSTORAGE_ERROR          RESULT_ERROR_SD_FILE_ERROR
#define SDSTORAGE_NOT_READY      RESULT_ERROR_SD_NOT_READY
#define SDSTORAGE_FILE_ERROR     RESULT_ERROR_SD_FILE_ERROR
#define SDSTORAGE_DISK_FULL      RESULT_ERROR_SD_DISK_FULL
#define SDSTORAGE_INVALID_PARAM  RESULT_ERROR_INVALID_PARAM

// 로그 파일 설정
#define SDSTORAGE_MAX_LOG_SIZE   (1024 * 1024)  // 1MB per log file
#define SDSTORAGE_LOG_DIR        "lora_logs"
#define SDSTORAGE_LOG_PREFIX     "lora_log_"
#define SDSTORAGE_LOG_EXTENSION  ".bin"

#endif // SDSTORAGE_H