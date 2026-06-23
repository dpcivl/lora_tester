<!--
  lora_tester 저장소의 README.md 초안입니다.
  저장소 루트에 README.md 로 추가하세요.
  [확인 필요] 표시된 부분은 실제 코드/환경에 맞게 채워주세요.
-->
LoRa Tester

STM32 기반 LoRa 모듈의 동작을 검증하기 위한 임베디드 테스트 도구입니다.
Unity 테스트 프레임워크와 Ceedling을 사용해 펌웨어 로직을 단위 테스트로 검증합니다.

개요

LoRa 통신 모듈의 명령 송수신, 상태 관리, 전력 관리 등을 호스트 환경에서 단위 테스트로 검증하고,
검증된 로직을 STM32 타겟에 올려 동작을 확인합니다.


테스트 우선(TDD) 방식으로 펌웨어 로직을 개발
하드웨어 없이도 호스트 PC에서 로직 검증 가능
STM32 타겟 빌드를 별도 디렉터리로 분리


기술 스택


언어: C
타겟 보드: STM32F746G-DISCO (STM32F7 시리즈)
테스트: Unity + Ceedling (project.yml)
LoRa 모듈: RAK3272S (LoRaWAN 모듈)
타겟 빌드: STM32CubeIDE


프로젝트 구조

lora_tester/
├── src/                  # 테스트 대상 소스 코드
├── test/                 # Unity 단위 테스트
├── lora_tester_stm32/    # STM32 타겟 빌드 프로젝트
├── project.yml           # Ceedling 설정
└── docs/                 # 설계 메모 및 테스트 가이드

시작하기

사전 요구사항


Ruby & Ceedling (gem install ceedling) — 호스트 단위 테스트용
STM32CubeIDE — STM32F746G-DISCO 타겟 빌드용


단위 테스트 실행

bash# 전체 테스트 실행
ceedling test:all

# 특정 테스트만 실행
ceedling test:<테스트파일명>

STM32 타겟 빌드


STM32CubeIDE에서 lora_tester_stm32/ 프로젝트를 import
STM32F746G-DISCO 보드를 연결
Build 후 보드에 플래시


테스트 항목


함수 단위 테스트
LoRa 명령 송수신 파싱
디바이스 상태 전이
전력 관리 로직 (power_management_test_guide.md 참고)


<!--
  라이선스: 개인 포트폴리오 repo라면 생략해도 무방합니다.
  넣고 싶으면 GitHub에서 "Add file > Create new file > LICENSE" 입력 시
  오른쪽에 "Choose a license template" 버튼이 뜹니다 (MIT 추천).
  추가했다면 아래 주석을 풀어 쓰세요.

## 라이선스

MIT License
-->
