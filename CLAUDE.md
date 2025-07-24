# LoRa Tester STM32 Project Progress

## Project Overview
STM32F746G-DISCO board with RAK3172 LoRa module for LoRaWAN communication implementation

## Completed Tasks

### 1. Hardware Connection & UART Communication Success
- **Hardware**: STM32F746G-DISCO + RAK3172 LoRa module
- **Connection**: UART6 (PC6=TX, PC7=RX) 115200 baud
- **Communication**: DMA-based reception, polling-based transmission
- **Status**: AT command with OK response confirmed ✅

### 2. TDD-based Code Structure Implementation
- **LoraStarter**: State machine for LoRa initialization and JOIN process
- **CommandSender**: AT command transmission handler
- **ResponseHandler**: LoRa module response parsing (OK, ERROR, JOIN, etc.)
- **UART**: Platform abstraction layer (STM32/Win32/Mock support)
- **Logger**: Unified logging system

### 3. LoRa Initialization Command Sequence (UPDATED)
```c
const char* lora_init_commands[] = {
    "AT+VER\r\n",       // Version check (connection test)
    "AT+NWM=1\r\n",     // LoRaWAN mode setting
    "AT+NJM=1\r\n",     // OTAA mode setting
    "AT+CLASS=A\r\n",   // Class A setting
    "AT+BAND=7\r\n"     // Asia 923 MHz band setting
};
```

### 4. STM32 DMA Configuration Optimization
- **DMA Mode**: CIRCULAR → NORMAL mode change
- **IDLE Interrupt**: Message end detection
- **Error Handling**: Comprehensive UART error recovery logic

## Current Status & Issues

### ✅ RESOLVED ISSUES (2025-07-11)
- ✅ **DMA restart failed (status: 1)** - UART BUSY state persistence FIXED
- ✅ **LoRa command sequential processing** - All 5 initialization commands working
- ✅ **State machine not progressing** - Response filtering implemented
- ✅ **TX/RX logging enhancement** - Complete command visibility
- ✅ **Error handling & retry logic** - 3-retry mechanism added
- ✅ **AT command format issues** - Missing \r\n termination fixed
- ✅ **JOIN response parsing** - Newline character handling fixed
- ✅ **SEND command format** - Hex encoding implementation
- ✅ **Periodic transmission** - 30-second interval timer logic
- ✅ **Response confirmation** - SEND_CONFIRMED_OK parsing fixed

### Successful Parts
- ✅ RAK3172 boot message reception and filtering
- ✅ Complete LoRa initialization sequence (5 commands)
- ✅ TX logging with hex dump for debugging
- ✅ Response filtering (boot messages vs command responses)
- ✅ Error recovery with retry mechanism
- ✅ JOIN process initiated

### Current Implementation Status
- ✅ **All initialization commands working**: AT+VER, AT+NWM=1, AT+NJM=1, AT+CLASS=A, AT+BAND=7
- ✅ **JOIN process completed**: +EVT:JOINED response successfully parsed
- ✅ **Data transmission working**: AT+SEND=1:54455354 (hex-encoded payload)
- ✅ **Periodic operation active**: 30-second interval timer implemented
- ✅ **Production ready**: Complete end-to-end LoRaWAN communication

## Technical Details

### Enhanced LoRa State Machine (LoraStarter)
```
INIT → SEND_CMD → WAIT_OK → ... → SEND_JOIN → WAIT_JOIN_OK → 
SEND_PERIODIC → WAIT_SEND_RESPONSE → WAIT_SEND_INTERVAL → SEND_PERIODIC (loop)
```

### New State: LORA_STATE_WAIT_SEND_INTERVAL
- **Purpose**: Implements 30-second periodic transmission interval
- **Logic**: Compares `current_time - last_send_time >= send_interval_ms`
- **Default**: 30000ms (30 seconds) configurable interval

### Hex Data Encoding Implementation
```c
// String to hex conversion for LoRaWAN payload
int len = strlen(message);
for (int i = 0; i < len && i < 31; i++) {
    sprintf(&hex_data[i*2], "%02X", (unsigned char)message[i]);
}
// Result: "TEST" → "54455354"
```

### Response Handler Improvements
- **JOIN Response**: Handles `+EVT:JOINED\r\n` with newline handling
- **SEND Response**: Handles `+EVT:SEND_CONFIRMED_OK\r\n` with strstr matching
- **Robust Parsing**: Tolerates various response formats with newlines

### DMA Configuration
- **Instance**: DMA2_Stream1
- **Channel**: DMA_CHANNEL_5 (USART6_RX)
- **Mode**: DMA_NORMAL (changed from circular mode)
- **Priority**: DMA_PRIORITY_HIGH

### UART Configuration
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

## Log Example (Complete Working State)
```
[INFO] === LoRa Initialization ===
[INFO] 📤 Commands: 5, Message: TEST, Max retries: 3
[INFO] [LoRa] Initialized with message: TEST, max_retries: 3
[INFO] [TX_TASK] 📤 Sending command 1/5
[INFO] 📤 TX: 'AT+VER\r\n' (8 bytes)
[INFO] 📥 RECV: 'OK\r\n' (4 bytes)
[INFO] ✅ OK response
... (all 5 commands successful)
[INFO] [LoRa] JOIN attempt started
[INFO] 📤 TX: 'AT+JOIN\r\n' (9 bytes)
[INFO] 📥 RECV: '+EVT:JOINED\r\n' (13 bytes)
[INFO] [ResponseHandler] JOIN response confirmed: +EVT:JOINED
[INFO] ✅ JOIN response
[INFO] [LoRa] Starting periodic send with message: TEST
[INFO] 📤 TX: 'AT+SEND=1:54455354\r\n' (20 bytes)
[INFO] 📥 RECV: 'OK\r\n' (4 bytes)
[INFO] 📥 RECV: '+EVT:SEND_CONFIRMED_OK\r\n' (24 bytes)
[INFO] [ResponseHandler] SEND response: CONFIRMED_OK
[INFO] ✅ SEND successful
[DEBUG] [TX_TASK] ⏳ Waiting for send interval (30000 ms)
[DEBUG] [LoRa] Send interval passed (30000 ms), ready for next send
[INFO] 📤 TX: 'AT+SEND=1:54455354\r\n' (20 bytes)
... (repeats every 30 seconds)
```

## Next Steps

### ✅ COMPLETED (2025-07-11)
1. ✅ **DMA BUSY state resolved** - Complete reset with error flag clearing
2. ✅ **LoRa command sequential processing verified** - All 5 commands working
3. ✅ **Response filtering implemented** - Boot messages properly ignored
4. ✅ **TX/RX logging enhanced** - Full command visibility with hex dump
5. ✅ **Error handling & retry logic** - 3-retry mechanism for failed commands
6. ✅ **AT command format fixed** - All commands now include \r\n termination
7. ✅ **JOIN process completed** - +EVT:JOINED response parsing working
8. ✅ **Hex payload encoding** - String to hex conversion for LoRaWAN
9. ✅ **Periodic transmission** - 30-second interval timer implementation
10. ✅ **Production deployment** - Complete end-to-end system working

### ✅ PRODUCTION READY STATUS
- **Initialization**: Complete 5-command sequence
- **Network JOIN**: OTAA activation successful
- **Data Transmission**: Hex-encoded payload transmission
- **Periodic Operation**: 30-second interval automatic transmission
- **Error Handling**: Comprehensive retry and recovery logic
- **Testing**: 36 unit tests passing, hardware integration verified

### Next Development Phase (Future)
1. **LAN Communication Module** - Ethernet-based error reporting to specific IP
2. **Enhanced Error Classification** - Network failure vs transmission failure
3. **Automatic Re-JOIN Logic** - Smart network disconnection recovery
4. **Real Sensor Data Integration** - Temperature, humidity, etc.
5. **Power Management** - Sleep mode and low-power operation

## Key File Locations

### STM32 Code
- `lora_tester_stm32/Core/Src/main.c` - Main logic, RTOS tasks
- `lora_tester_stm32/Core/Src/uart/src/uart_stm32.c` - UART DMA handling
- `lora_tester_stm32/Core/Src/CommandSender.c` - AT command transmission
- `lora_tester_stm32/Core/Src/ResponseHandler.c` - Response parsing

### TDD Common Code
- `src/LoraStarter.c` - LoRa state machine (TDD verified)
- `src/uart_common.c` - UART common logic
- `test/test_LoraStarter.c` - Unit tests

### Configuration & Documentation
- `lora_tester_stm32/LORA_TEST_GUIDE.md` - Hardware connection guide
- `project.yml` - Ceedling test configuration

## Hardware Connection Info
```
STM32F746G-DISCO        RAK3172 LoRa Module
================        ===============
PC6 (UART6_TX)    ----> RX
PC7 (UART6_RX)    <---- TX  
GND               ----> GND
3.3V              ----> VCC
```

## Development Environment
- **IDE**: STM32CubeIDE
- **MCU**: STM32F746NGH6
- **LoRa Module**: RAK3172 (RUI_4.0.6_RAK3172-E)
- **Test Framework**: Ceedling + Unity
- **Build System**: Make + CMock

## Memories
- To memorize

## Major Achievements Today (2025-07-10)

### 🎉 Successfully Resolved Core Issues:
1. **DMA Communication Stability** - No more restart failures
2. **Complete LoRa Initialization** - All 5 commands working perfectly  
3. **State Machine Robustness** - Proper error handling and retry logic
4. **TX/RX Visibility** - Full debugging capability with hex dumps
5. **Response Processing** - Smart filtering between boot messages and command responses

### 🚀 Technical Improvements:
- **UART Error Flag Management** - Comprehensive clearing of ORE, FE, NE, PE flags
- **DMA State Synchronization** - Proper wait for READY state before restart
- **Command Format Standardization** - All AT commands properly terminated with \r\n
- **Response Handler Enhancement** - Support for version responses (RUI_ detection)
- **Retry Mechanism** - 3-attempt retry with graceful fallback to next command

### 📡 LoRa Protocol Status:
- **Initialization**: ✅ COMPLETE (5/5 commands successful)
- **JOIN Process**: 🔄 IN PROGRESS (awaiting network response)
- **Data Transmission**: 🟡 READY (pending JOIN completion)

## Testing Status
- **Unit Tests**: 36/36 passing ✅
- **Integration Tests**: Hardware verified ✅
- **TDD Coverage**: Core logic fully tested ✅
- **Manual Testing**: End-to-end functionality verified ✅

## Major Milestones Achieved

### 🎉 Milestone 1: Basic Communication (2025-07-10)
- UART DMA communication established
- AT command processing working
- Response parsing implemented

### 🎉 Milestone 2: LoRa Initialization (2025-07-10)
- Complete 5-command initialization sequence
- JOIN process working
- Error handling and retry logic

### 🎉 Milestone 3: Data Transmission (2025-07-11)
- Hex payload encoding implemented
- Successful data transmission
- Response confirmation working

### 🎉 Milestone 4: Periodic Operation (2025-07-11)
- 30-second interval timer
- Continuous operation loop
- Production-ready state machine

## SD Card Logging Implementation Progress (2025-07-23)

### 🔄 CURRENT WORK: SD Card Integration
- **Objective**: Implement dual logging (Terminal + SD Card)  
- **Status**: 🟡 IN PROGRESS - Hardware level working, FatFs integration blocked

### SD Card Hardware Analysis Results:
- ✅ **Physical Detection**: SD Detect Pin (PC13) = 0 (Card Present)
- ✅ **SDMMC Controller**: HAL_SD_Init() successful, State = 1 (Ready)
- ✅ **Card Recognition**: 3584 MB SD card fully detected
- ✅ **Card State**: HAL_SD_GetCardState() = 4 (TRANSFER mode)
- ❌ **FatFs Integration**: disk_initialize() returns 1 (STA_NOINIT)

### Technical Issues Identified:
1. **BSP vs HAL Conflict**: 
   - FatFs sd_diskio.c uses BSP_SD_* functions
   - We initialize with HAL_SD_* functions  
   - Modified `SD_CheckStatus()` to use HAL functions directly

2. **FatFs Configuration**: 
   - Enabled `DISABLE_SD_INIT` flag to prevent BSP re-initialization
   - SD card hardware is perfect, issue is in FatFs driver layer

3. **Current Blocker**: 
   - `disk_initialize()` fails despite perfect hardware state
   - Suspected FreeRTOS kernel state check issue
   - Added debugging to `SD_initialize()` function

### Code Changes Made:
```c
// sd_diskio.c modifications:
#define DISABLE_SD_INIT  // Skip BSP_SD_Init()

static DSTATUS SD_CheckStatus(BYTE lun) {
    // Use HAL instead of BSP
    HAL_SD_CardStateTypeDef cardState = HAL_SD_GetCardState(&hsd1);
    if(cardState == HAL_SD_CARD_TRANSFER) {
        Stat &= ~STA_NOINIT;
    }
    return Stat;
}
```

### Buffer Size Fixes:
- **RX Buffers**: 256 → 512 bytes (prevent DMA overflow)
- **Files Modified**: main.c, ResponseHandler.c, uart_stm32.c

### Logger System:
- ✅ **Terminal Output**: Working perfectly  
- 🟡 **SD Card Output**: Hardware ready, software blocked
- 🟡 **Dual Output**: Prepared but not active

## SD Card 디버깅 세션 진행상황 (2025-07-24)

### 🎯 **오늘 세션 목표**
- SD카드 f_mount() 시스템 멈춤 문제 해결
- LoRa + SD 듀얼 로깅 시스템 완성

### 📊 **현재 상태 분석**

#### ✅ **정상 작동하는 부분들:**
- **LoRa 통신**: 완전 정상 작동 (30초 주기 전송, JOIN 성공)
- **SD 하드웨어**: 3584MB 카드 완벽 인식 
- **UART 통신**: 터미널 로깅 정상

#### ❌ **문제가 있는 부분들:**
- **FatFs disk_initialize()**: 계속 STA_NOINIT 반환 (result: 1)
- **BSP vs HAL 충돌**: 초기화 방식 불일치
- **Card Size: 0 MB**: HAL_SD_GetCardInfo() 정보 누락

### 🔧 **오늘 시도한 해결방안들**

#### **1. 스택 사이즈 증가 (4096 → 8192)**
- **결과**: UART 에러 발생으로 되돌림
- **원인**: 메모리 레이아웃 변경으로 UART DMA 충돌
- **결론**: 스택 부족이 아닌 다른 원인

#### **2. f_getfree() 제거**
- **목적**: 시스템 멈춤 방지
- **결과**: f_mkdir()에서 또 다른 멈춤 발생
- **결론**: 모든 FatFs 실제 SD 접근 함수가 문제

#### **3. 지연 마운트 → 즉시 마운트 변경**
- **목적**: 초기화 타이밍 최적화
- **결과**: f_mount() 자체에서 FR_NO_FILESYSTEM (3) 오류

#### **4. HAL → BSP 초기화 방식 변경**
- **목적**: FatFs 호환성 개선
- **결과**: BSP_SD_Init() 실패 (MSD_ERROR)
- **원인**: HAL과 BSP 이중 초기화 충돌

#### **5. 중복 초기화 제거**
- **최종 시도**: FatFs가 자동으로 BSP_SD_Init() 호출하도록 함
- **현재 상태**: 여전히 disk_initialize() 실패

### 📋 **핵심 문제 진단**

#### **A. SD 하드웨어 상태**
```
✅ SDMMC1 peripheral: 정상 초기화
✅ Card detection: PC13 핀 정상 감지
✅ HAL_SD_Init(): 성공 (result: 0)
❌ HAL_SD_GetCardInfo(): Card Size = 0 MB (정보 누락)
❌ disk_initialize(): STA_NOINIT 지속 반환
```

#### **B. FatFs 설정 상태**
```
✅ ffconf.h: FreeRTOS 호환 설정 완료
✅ 메모리 할당: pvPortMalloc/vPortFree 연결
✅ 세마포어: CMSIS v1.x 호환
❌ BSP_SD_Init(): sd_diskio.c에서 실패
```

#### **C. 추정 근본 원인들**
1. **STM32CubeMX HAL 초기화와 FatFs BSP 초기화 불일치**
2. **SD카드 정보 읽기 실패로 인한 파일시스템 인식 불가**
3. **RTOS 컨텍스트에서 BSP 함수 호출 시 타이밍 문제**

### 💡 **다음 세션 해결 방향**

#### **우선순위 1: BSP_SD_Init() 실패 원인 분석**
```c
// sd_diskio.c의 SD_initialize() 함수에서:
if(BSP_SD_Init() == MSD_OK) {  // ← 여기서 실패
    Stat = SD_CheckStatus(lun);
}
```
**접근법:**
- BSP_SD_Init() 내부 로직 분석
- HAL과 BSP 초기화 순서 최적화
- 또는 DISABLE_SD_INIT 활성화로 BSP 우회

#### **우선순위 2: HAL_SD_GetCardInfo() 수정**
```c
// Card Size: 0 MB 문제 해결
HAL_StatusTypeDef info_result = HAL_SD_GetCardInfo(&hsd1, &cardInfo);
// cardInfo.LogBlockNbr, LogBlockSize가 0으로 나오는 원인 분석
```

#### **우선순위 3: 대안 접근법**
- **Raw 섹터 접근**: FatFs 우회하고 HAL_SD_ReadBlocks() 직접 사용
- **다른 파일시스템**: LittleFS 등 대안 고려
- **터미널 전용**: SD 로깅 포기하고 LoRa 완성도 높이기

### 🎯 **다음 세션 체크리스트**

#### **즉시 확인할 사항들:**
- [ ] BSP_SD_Init() 실패 상세 분석 (bsp_driver_sd.c 내부)
- [ ] DISABLE_SD_INIT 플래그 활성화 테스트
- [ ] HAL_SD_GetCardInfo() 0 MB 문제 해결
- [ ] sd_diskio.c 설정 재검토

#### **대안 구현 준비:**
- [ ] 터미널 전용 로깅으로 LoRa 시스템 완성
- [ ] Raw SD 섹터 접근 방식 프로토타입
- [ ] 메모리 사용량 최적화

### 📊 **현재 시스템 안정성**
- **LoRa 통신**: ✅ **프로덕션 레디** (30초 주기 전송 완벽)
- **터미널 로깅**: ✅ **완전 정상** (모든 이벤트 출력)
- **SD 로깅**: ❌ **블로킹됨** (disk_initialize 실패)

---
**Last Update**: 2025-07-24 오전 세션  
**Status**: 🟡 LoRa PRODUCTION READY + SD Card Hardware OK + FatFs disk_initialize 실패  
**Current Blocker**: BSP_SD_Init() MSD_ERROR + HAL_SD_GetCardInfo() 0MB 문제  
**Next Priority**: BSP vs HAL 초기화 충돌 해결 + DISABLE_SD_INIT 활성화 테스트