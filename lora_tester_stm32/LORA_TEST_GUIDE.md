# STM32F746G-DISCO LoRa 모듈 연결 테스트 가이드

## 🔧 **하드웨어 연결**

### **STM32F746G-DISCO 보드**
- **UART6 핀 위치**:
  - **PC6**: UART6_TX (Arduino 헤더 D1)
  - **PC7**: UART6_RX (Arduino 헤더 D0)
  - **GND**: 그라운드 (Arduino 헤더 GND)
  - **3.3V**: 전원 (Arduino 헤더 3.3V)

### **LoRa 모듈 연결**
```
STM32F746G-DISCO        LoRa 모듈
================        =========
PC6 (UART6_TX)    ----> RX
PC7 (UART6_RX)    <---- TX
GND               ----> GND
3.3V              ----> VCC
```

## 🚀 **빌드 및 실행**

### **1. 프로젝트 빌드**
1. STM32CubeIDE 실행
2. `lora_tester_stm32` 프로젝트 열기
3. `Project → Clean → Build` 실행

### **2. 펌웨어 업로드**
1. STM32F746G-DISCO 보드를 PC에 연결
2. `Run → Debug` 또는 `Run → Run` 실행
3. 펌웨어가 보드에 업로드됨

## 🔍 **연결 테스트 방법**

### **1. 기본 AT 명령어 테스트**
펌웨어가 실행되면 다음 순서로 테스트가 진행됩니다:

```
1. AT          (모듈 응답 확인)
2. AT+VER      (버전 정보 확인)
3. AT+ID       (ID 정보 확인)
4. AT+JOIN     (네트워크 JOIN 시도)
5. AT+SEND=TEST (테스트 데이터 전송)
```

### **2. 로그 모니터링**
#### **ST-Link Virtual COM Port 사용**
- **보드레이트**: 115200
- **데이터 비트**: 8
- **스톱 비트**: 1
- **패리티**: None

#### **시리얼 터미널 추천**
- **Windows**: PuTTY, Tera Term
- **Linux/Mac**: screen, minicom

### **3. 예상 로그 출력**
```
[INFO] [STM32] LoRa Tester Started
[INFO] [UART] Connecting to UART6
[INFO] [UART] Successfully connected to UART6
[INFO] [LoRa] UART connected to UART6
[INFO] [LoRa] Initialized with message: TEST, max_retries: 3
[DEBUG] [LoRa] Sending command 1/3: AT
[DEBUG] [UART] Sending data: AT
[DEBUG] [UART] Received 3 bytes: OK
[DEBUG] [LoRa] Command 1 OK received
[DEBUG] [LoRa] Sending command 2/3: AT+VER
...
```

## 🔧 **문제 해결**

### **1. 연결 문제**
- **증상**: "UART Connect failed" 또는 응답 없음
- **해결책**:
  - 하드웨어 연결 확인
  - LoRa 모듈 전원 확인
  - UART 핀 매핑 확인

### **2. 응답 없음**
- **증상**: AT 명령어 전송 후 응답 없음
- **해결책**:
  - LoRa 모듈 보드레이트 확인 (115200)
  - TX/RX 핀 연결 확인
  - 모듈 초기화 시간 대기

### **3. 로그 출력 없음**
- **증상**: 시리얼 터미널에 로그가 표시되지 않음
- **해결책**:
  - Virtual COM Port 드라이버 설치
  - 올바른 COM 포트 선택
  - 보드레이트 설정 확인

## 📋 **지원되는 LoRa 모듈**

### **테스트된 모듈**
- **RAK3172**: 기본 AT 명령어 지원
- **RAK4600**: 기본 AT 명령어 지원
- **기타 AT 명령어 호환 모듈**

### **AT 명령어 참고**
- `AT`: 모듈 응답 확인
- `AT+VER`: 펌웨어 버전 확인
- `AT+ID`: 디바이스 ID 확인
- `AT+JOIN`: LoRaWAN 네트워크 JOIN
- `AT+SEND=data`: 데이터 전송

## 🎯 **다음 단계**

### **1. 고급 테스트**
- JOIN 재시도 로직 테스트
- 네트워크 연결 안정성 테스트
- 다양한 데이터 크기 전송 테스트

### **2. 설정 커스터마이징**
- `main.c`에서 테스트 명령어 변경
- 재시도 횟수 조정
- 전송 간격 조정

### **3. 실제 LoRaWAN 네트워크 연결**
- The Things Network (TTN) 설정
- 실제 센서 데이터 전송
- 양방향 통신 테스트

---

**참고**: 이 가이드는 기본 연결 테스트용입니다. 실제 운용 환경에서는 추가 설정이 필요할 수 있습니다. 