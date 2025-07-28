# LoRa Tester 프로젝트 개발 규칙

## 코드 수정 금지 영역

### 1. FATFS/Target 디렉토리
**절대 수정 금지**: `lora_tester_stm32/FATFS/Target/` 내의 모든 파일
- `sd_diskio.c`
- `bsp_driver_sd.c`  
- `fatfs_platform.c`
- 기타 모든 FatFs 관련 파일

**이유**: STM32 IDE(CubeMX)에서 자동 생성한 코드
**원칙**: 꼭 필요한 경우 외에는 수정하지 않고 갖다 쓰기

### 2. STM32 HAL/Driver 코드
**절대 수정 금지**: `Drivers/` 디렉토리 내의 모든 HAL 드라이버 코드
**이유**: STM32 공식 HAL 라이브러리

## 수정 가능 영역

### 1. 애플리케이션 코드
- `Core/Src/main.c` - 메인 애플리케이션 로직
- `Core/Src/SDStorage.c` - 사용자 정의 SD 스토리지 모듈
- `Core/Src/uart/` - 사용자 정의 UART 모듈
- `src/` - TDD 검증된 공통 모듈들

### 2. 설정 파일
- `Core/Inc/` - 사용자 정의 헤더 파일
- `CLAUDE.md` - 프로젝트 진행 상황 문서

## 개발 원칙

1. **STM32 IDE 생성 코드 존중**: 자동 생성 코드는 있는 그대로 활용
2. **TDD 모듈 우선 활용**: 검증된 TDD 모듈을 최대한 활용
3. **상위 레벨에서 통합**: 하위 레벨 코드 수정보다는 상위 애플리케이션 레벨에서 문제 해결
4. **BSP 활용**: HAL 직접 사용보다는 BSP 레이어 활용

## 위반 시 조치

- FATFS/Target 코드 수정 시도 시 즉시 중단
- 대안적 해결 방법 모색 (상위 레벨 코드 수정)
- 사용자에게 확인 후 진행

---
**생성일**: 2025-07-25
**최종 수정**: 2025-07-25