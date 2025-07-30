/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"
#include "stdio.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_host.h"
#include "bsp_driver_sd.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "logger.h"
#include "uart.h"
#include "LoraStarter.h"
#include "CommandSender.h"
#include "ResponseHandler.h"
#include "Network.h"
#include "SDStorage.h"

// LoraStarter용 로깅 매크로는 logger.h에 정의되어 있음

// UART 전역 변수들 (uart_stm32.c에서 정의됨)
extern char rx_buffer[512];
extern volatile uint16_t uart_rx_length;
extern volatile uint8_t uart_rx_complete_flag;
extern volatile uint8_t uart_rx_error_flag;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2004c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2004c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2004c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2004c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

ETH_TxPacketConfig TxConfig;

ADC_HandleTypeDef hadc3;

CRC_HandleTypeDef hcrc;

DCMI_HandleTypeDef hdcmi;

DMA2D_HandleTypeDef hdma2d;

ETH_HandleTypeDef heth;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

QSPI_HandleTypeDef hqspi;

RTC_HandleTypeDef hrtc;

SAI_HandleTypeDef hsai_BlockA2;
SAI_HandleTypeDef hsai_BlockB2;

SD_HandleTypeDef hsd1;

SPDIFRX_HandleTypeDef hspdif;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim12;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

SDRAM_HandleTypeDef hsdram1;

osThreadId defaultTaskHandle;
osThreadId receiveTaskHandle;
osThreadId sdLoggingTaskHandle;  // SD 로깅 전용 태스크

// SD 로깅 큐 및 상태 관리 (메모리 최적화)
#define SD_LOG_QUEUE_SIZE 10         // 50 → 10으로 축소
#define SD_LOG_MAX_MESSAGE_SIZE 128  // 512 → 128로 축소

typedef struct {
    char message[SD_LOG_MAX_MESSAGE_SIZE];
    uint32_t timestamp;
    size_t length;
} SDLogEntry_t;

osMessageQId sdLogQueueHandle;
static bool g_sd_logging_active = false;

/* USER CODE BEGIN PV */

// 수신 태스크용 전역 변수
int rx_bytes_received = 0;
osMessageQId rxMessageQueue;

// LoRa 통신용 전역 변수
char lora_rx_response[512];
volatile bool lora_new_response = false;

// DMA 관련 변수
DMA_HandleTypeDef hdma_usart6_rx;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC3_Init(void);
static void MX_CRC_Init(void);
static void MX_DCMI_Init(void);
static void MX_DMA2D_Init(void);
static void MX_ETH_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C3_Init(void);
static void MX_LTDC_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_RTC_Init(void);
static void MX_SAI2_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_SPDIFRX_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM12_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART6_UART_Init(void);
void MX_USART6_DMA_Init(void);  // USART6 DMA 초기화 함수 선언
void StartDefaultTask(void const * argument);
void StartSDLoggingTask(void const * argument);
void StartReceiveTask(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// UART DMA 콜백 함수들은 uart_stm32.c로 이동됨

// SD 카드 초기화 결과를 저장하는 전역 변수
int g_sd_initialization_result = -1;  // -1: 초기화 안됨, SDSTORAGE_OK: 성공, 기타: 실패

// 시간 동기화 관리 변수
static bool g_time_request_sent = false;
static uint32_t g_join_success_time = 0;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  // 리셋 카운터 추가
  static uint32_t reset_count = 0;
  reset_count++;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();  // DMA는 UART보다 먼저 초기화
  MX_USART6_DMA_Init();  // USART6 DMA 초기화 (UART보다 먼저)
  MX_ADC3_Init();
  MX_CRC_Init();
  MX_DCMI_Init();
  MX_DMA2D_Init();
  MX_ETH_Init();
  MX_FMC_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_QUADSPI_Init();
  MX_RTC_Init();
  MX_SAI2_Init();
  MX_SDMMC1_SD_Init();
  MX_SPDIFRX_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM5_Init();
  MX_TIM8_Init();
  MX_TIM12_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  
  // UART 초기화 후 DMA 핸들 다시 연결 (HAL_UART_Init에서 리셋될 수 있음)
  __HAL_LINKDMA(&huart6, hdmarx, hdma_usart6_rx);
  
  // UART IDLE 인터럽트 활성화 (DMA 기반 수신을 위해)
  __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  
  // Logger 초기화 (터미널 출력만 사용)
  LOGGER_Connect("STM32", 0);
  
  // 리셋 원인 확인
  LOG_INFO("=== SYSTEM START (Reset #%lu) ===", reset_count);
  
  // RCC 리셋 플래그 확인
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST)) LOG_WARN("Reset: BOR (Brown-out)");
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) LOG_WARN("Reset: PIN (External)");
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) LOG_WARN("Reset: POR (Power-on)");
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) LOG_WARN("Reset: SOFTWARE");
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) LOG_WARN("Reset: IWDG (Watchdog)");
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) LOG_WARN("Reset: WWDG (Window Watchdog)");
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST)) LOG_WARN("Reset: LPWR (Low Power)");
  
  // 플래그 클리어
  __HAL_RCC_CLEAR_RESET_FLAGS();
  
  // ===== 하드웨어 초기화만 main()에서 수행 =====
  
  // SD카드 초기화는 FreeRTOS 태스크에서 수행 (커널 시작 후)
  LOG_INFO("🔄 SD card initialization will be performed in FreeRTOS task");
  g_sd_initialization_result = -1;  // 초기화 안됨 상태
  
  // UART6 DMA 초기화 건너뛰기 (이미 main 초기화에서 완료됨)
  LOG_INFO("📤 UART DMA already initialized in main() - skipping");
  
  // IDLE 인터럽트만 활성화 (메시지 끝 감지용)
  LOG_INFO("📤 Enabling UART IDLE interrupt...");
  __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
  LOG_INFO("✅ UART setup completed");

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  // SD 로깅 큐 생성 (안전성 체크 포함)
  LOG_INFO("📤 Creating SD logging queue (size: %d, item: %d bytes)", 
           SD_LOG_QUEUE_SIZE, sizeof(SDLogEntry_t));
  
  osMessageQDef(sdLogQueue, SD_LOG_QUEUE_SIZE, SDLogEntry_t);
  sdLogQueueHandle = osMessageCreate(osMessageQ(sdLogQueue), NULL);
  
  if (sdLogQueueHandle == NULL) {
    LOG_ERROR("❌ SD logging queue creation FAILED - insufficient memory");
  } else {
    LOG_INFO("✅ SD logging queue created successfully");
  }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 8192);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* 수신 태스크 활성화 - LoRa 통신을 위해 필수 */
  osThreadDef(receiveTask, StartReceiveTask, osPriorityNormal, 0, 4096);
  receiveTaskHandle = osThreadCreate(osThread(receiveTask), NULL);
  LOG_INFO("📤 Receive Task enabled for LoRa communication");
  
  /* SD 로깅 태스크 활성화 - SD 카드 로깅을 위해 */
  osThreadDef(sdLoggingTask, StartSDLoggingTask, osPriorityLow, 0, 4096);
  sdLoggingTaskHandle = osThreadCreate(osThread(sdLoggingTask), NULL);
  LOG_INFO("📤 SD Logging Task enabled");
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  LOG_INFO("🚀 Starting FreeRTOS scheduler...");
  osKernelStart();
  
  // 이 부분은 절대 실행되면 안됨 (스케줄러가 제어를 가져가야 함)
  LOG_ERROR("❌ FATAL: Scheduler failed to start - system halted");

  /* We should never get here as control is now taken by the scheduler */
  
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SAI2
                              |RCC_PERIPHCLK_SDMMC1|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  PeriphClkInitStruct.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLSAI;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DCMI Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_FALLING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00C0EAFF;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00C0EAFF;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 40;
  hltdc.Init.VerticalSync = 9;
  hltdc.Init.AccumulatedHBP = 53;
  hltdc.Init.AccumulatedVBP = 11;
  hltdc.Init.AccumulatedActiveW = 533;
  hltdc.Init.AccumulatedActiveH = 283;
  hltdc.Init.TotalWidth = 565;
  hltdc.Init.TotalHeigh = 285;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 480;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 272;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = 0xC0000000;
  pLayerCfg.ImageWidth = 480;
  pLayerCfg.ImageHeight = 272;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold = 4;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hqspi.Init.FlashSize = 24;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_6_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  RTC_AlarmTypeDef sAlarm = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the Alarm A
  */
  sAlarm.AlarmTime.Hours = 0x0;
  sAlarm.AlarmTime.Minutes = 0x0;
  sAlarm.AlarmTime.Seconds = 0x0;
  sAlarm.AlarmTime.SubSeconds = 0x0;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 0x1;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the Alarm B
  */
  sAlarm.Alarm = RTC_ALARM_B;
  if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the TimeStamp
  */
  if (HAL_RTCEx_SetTimeStamp(&hrtc, RTC_TIMESTAMPEDGE_RISING, RTC_TIMESTAMPPIN_POS1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SAI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI2_Init(void)
{

  /* USER CODE BEGIN SAI2_Init 0 */

  /* USER CODE END SAI2_Init 0 */

  /* USER CODE BEGIN SAI2_Init 1 */

  /* USER CODE END SAI2_Init 1 */
  hsai_BlockA2.Instance = SAI2_Block_A;
  hsai_BlockA2.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockA2.Init.AudioMode = SAI_MODEMASTER_TX;
  hsai_BlockA2.Init.DataSize = SAI_DATASIZE_8;
  hsai_BlockA2.Init.FirstBit = SAI_FIRSTBIT_MSB;
  hsai_BlockA2.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockA2.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA2.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockA2.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_192K;
  hsai_BlockA2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockA2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockA2.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  hsai_BlockA2.FrameInit.FrameLength = 8;
  hsai_BlockA2.FrameInit.ActiveFrameLength = 1;
  hsai_BlockA2.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockA2.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
  hsai_BlockA2.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockA2.SlotInit.FirstBitOffset = 0;
  hsai_BlockA2.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  hsai_BlockA2.SlotInit.SlotNumber = 1;
  hsai_BlockA2.SlotInit.SlotActive = 0x00000000;
  if (HAL_SAI_Init(&hsai_BlockA2) != HAL_OK)
  {
    Error_Handler();
  }
  hsai_BlockB2.Instance = SAI2_Block_B;
  hsai_BlockB2.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockB2.Init.AudioMode = SAI_MODESLAVE_RX;
  hsai_BlockB2.Init.DataSize = SAI_DATASIZE_8;
  hsai_BlockB2.Init.FirstBit = SAI_FIRSTBIT_MSB;
  hsai_BlockB2.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockB2.Init.Synchro = SAI_SYNCHRONOUS;
  hsai_BlockB2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockB2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockB2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockB2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockB2.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockB2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  hsai_BlockB2.FrameInit.FrameLength = 8;
  hsai_BlockB2.FrameInit.ActiveFrameLength = 1;
  hsai_BlockB2.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockB2.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
  hsai_BlockB2.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockB2.SlotInit.FirstBitOffset = 0;
  hsai_BlockB2.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  hsai_BlockB2.SlotInit.SlotNumber = 1;
  hsai_BlockB2.SlotInit.SlotActive = 0x00000000;
  if (HAL_SAI_Init(&hsai_BlockB2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI2_Init 2 */

  /* USER CODE END SAI2_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;  // ST 커뮤니티 가이드: 1-bit 모드로 변경
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;  // 하드웨어 플로우 컨트롤 활성화 (안정성 향상)
  hsd1.Init.ClockDiv = 8;  // 클럭 분주비 증가 (2→8, STM32F7 안정화 권장값)
  /* USER CODE BEGIN SDMMC1_Init 2 */
  
  // Initialize SD card with HAL
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  
  // BSP 초기화도 호출 (FatFs 호환성을 위해)
  uint8_t bsp_result = BSP_SD_Init();
  if (bsp_result != MSD_OK) {
    // BSP 초기화 실패 시 로그만 출력 (HAL은 이미 성공했으므로 계속 진행)
    // Error_Handler() 호출하지 않음
  }

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief SPDIFRX Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPDIFRX_Init(void)
{

  /* USER CODE BEGIN SPDIFRX_Init 0 */

  /* USER CODE END SPDIFRX_Init 0 */

  /* USER CODE BEGIN SPDIFRX_Init 1 */

  /* USER CODE END SPDIFRX_Init 1 */
  hspdif.Instance = SPDIFRX;
  hspdif.Init.InputSelection = SPDIFRX_INPUT_IN0;
  hspdif.Init.Retries = SPDIFRX_MAXRETRIES_NONE;
  hspdif.Init.WaitForActivity = SPDIFRX_WAITFORACTIVITY_OFF;
  hspdif.Init.ChannelSelection = SPDIFRX_CHANNEL_A;
  hspdif.Init.DataFormat = SPDIFRX_DATAFORMAT_LSB;
  hspdif.Init.StereoMode = SPDIFRX_STEREOMODE_DISABLE;
  hspdif.Init.PreambleTypeMask = SPDIFRX_PREAMBLETYPEMASK_OFF;
  hspdif.Init.ChannelStatusMask = SPDIFRX_CHANNELSTATUS_OFF;
  hspdif.Init.ValidityBitMask = SPDIFRX_VALIDITYMASK_OFF;
  hspdif.Init.ParityErrorMask = SPDIFRX_PARITYERRORMASK_OFF;
  if (HAL_SPDIFRX_Init(&hspdif) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPDIFRX_Init 2 */

  /* USER CODE END SPDIFRX_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_MspPostInit(&htim5);

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 65535;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief TIM12 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM12_Init(void)
{

  /* USER CODE BEGIN TIM12_Init 0 */

  /* USER CODE END TIM12_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM12_Init 1 */

  /* USER CODE END TIM12_Init 1 */
  htim12.Instance = TIM12;
  htim12.Init.Prescaler = 0;
  htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim12.Init.Period = 65535;
  htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim12) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM12_Init 2 */

  /* USER CODE END TIM12_Init 2 */
  HAL_TIM_MspPostInit(&htim12);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK1;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, ARDUINO_D7_Pin|ARDUINO_D8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DCMI_PWR_EN_GPIO_Port, DCMI_PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, ARDUINO_D4_Pin|ARDUINO_D2_Pin|EXT_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : OTG_HS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_HS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_HS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_D7_Pin ULPI_D6_Pin ULPI_D5_Pin ULPI_D3_Pin
                           ULPI_D2_Pin ULPI_D1_Pin ULPI_D4_Pin */
  GPIO_InitStruct.Pin = ULPI_D7_Pin|ULPI_D6_Pin|ULPI_D5_Pin|ULPI_D3_Pin
                          |ULPI_D2_Pin|ULPI_D1_Pin|ULPI_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_VBUS_Pin */
  GPIO_InitStruct.Pin = OTG_FS_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Audio_INT_Pin */
  GPIO_InitStruct.Pin = Audio_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Audio_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARDUINO_D7_Pin ARDUINO_D8_Pin LCD_DISP_Pin */
  GPIO_InitStruct.Pin = ARDUINO_D7_Pin|ARDUINO_D8_Pin|LCD_DISP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : uSD_Detect_Pin */
  GPIO_InitStruct.Pin = uSD_Detect_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(uSD_Detect_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_BL_CTRL_Pin */
  GPIO_InitStruct.Pin = LCD_BL_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TP3_Pin NC2_Pin */
  GPIO_InitStruct.Pin = TP3_Pin|NC2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : DCMI_PWR_EN_Pin */
  GPIO_InitStruct.Pin = DCMI_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DCMI_PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_INT_Pin */
  GPIO_InitStruct.Pin = LCD_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ULPI_NXT_Pin */
  GPIO_InitStruct.Pin = ULPI_NXT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(ULPI_NXT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARDUINO_D4_Pin ARDUINO_D2_Pin EXT_RST_Pin */
  GPIO_InitStruct.Pin = ARDUINO_D4_Pin|ARDUINO_D2_Pin|EXT_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_STP_Pin ULPI_DIR_Pin */
  GPIO_InitStruct.Pin = ULPI_STP_Pin|ULPI_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_RXER_Pin */
  GPIO_InitStruct.Pin = RMII_RXER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RMII_RXER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_CLK_Pin ULPI_D0_Pin */
  GPIO_InitStruct.Pin = ULPI_CLK_Pin|ULPI_D0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_HOST - 임시 비활성화 (SD 카드 테스트용) */
  LOG_WARN("USB Host initialization temporarily disabled to avoid RTOS task conflicts");
  LOG_INFO("This eliminates USBH_Thread vs defaultTask priority conflicts");
  // MX_USB_HOST_Init();  // SD 카드 테스트 완료 후 재활성화 예정
  /* USER CODE BEGIN 5 */
  
  // SD Card 초기화는 이미 main()에서 우선 완료됨
  LOG_INFO("📋 SD card initialization completed in main() - checking status...");
  
  LOG_INFO("=== STM32F746G-DISCO UART6 Test Started ===");
  LOG_INFO("System Clock: %lu MHz", SystemCoreClock / 1000000);
  LOG_INFO("UART6 Configuration: 115200 baud, 8N1");
  LOG_INFO("📌 CRITICAL: For loopback test, connect PC6(TX) to PC7(RX) with a wire!");
  LOG_INFO("📌 UART6 Pins: PC6(TX) = Arduino D1, PC7(RX) = Arduino D0");
  
  // SD 카드 기본 기능 테스트 (Default Task에서 수행)
  LOG_INFO("📤 [TX_TASK] Starting SD card basic functionality test...");
  
  // SD 초기화 시도
  LOG_INFO("📤 [TX_TASK] Attempting SD card initialization...");
  g_sd_initialization_result = SDStorage_Init();
  
  if (g_sd_initialization_result == SDSTORAGE_OK) {
    LOG_INFO("✅ [TX_TASK] SD card initialization SUCCESS");
    
    // 기본 쓰기 테스트
    LOG_INFO("📤 [TX_TASK] Testing SD card write operation...");
    const char* test_message = "SD Card Test - Hello World from FreeRTOS!\n";
    int write_result = SDStorage_WriteLog(test_message, strlen(test_message));
    
    if (write_result == SDSTORAGE_OK) {
      LOG_INFO("✅ [TX_TASK] SD card write operation SUCCESS");
      LOG_INFO("🎉 [TX_TASK] SD card functionality confirmed - ready for long-term logging");
    } else {
      LOG_ERROR("❌ [TX_TASK] SD card write operation FAILED (code: %d)", write_result);
    }
  } else {
    LOG_ERROR("❌ [TX_TASK] SD card initialization FAILED (code: %d)", g_sd_initialization_result);
    LOG_INFO("📺 [TX_TASK] Continuing with terminal-only logging");
  }

  // SD 테스트 건너뛰고 바로 LoRa 시작
  
  LOG_INFO("📤 [TX_TASK] Starting LoRa initialization and JOIN...");
  
  // UART 연결 (LoRa 통신을 위해 필수)
  LOG_INFO("📤 [TX_TASK] Connecting to UART for LoRa communication...");
  UartStatus uart_status = UART_Connect("UART6");
  if (uart_status == UART_STATUS_OK) {
    LOG_INFO("✅ [TX_TASK] UART connection successful");
  } else {
    LOG_ERROR("❌ [TX_TASK] UART connection failed (status: %d)", uart_status);
  }
  
  LOG_INFO("📤 [TX_TASK] Waiting for LoRa module boot-up (5 seconds - optimized for long-term test)...");
  osDelay(5000); // 5초 대기 (장기 테스트를 위해 단축)
  
  // LoraStarter 컨텍스트 초기화 (TDD 검증된 기본 설정 사용)
  LoraStarterContext lora_ctx;
  LoraStarter_InitWithDefaults(&lora_ctx, "TEST");
  
  LOG_INFO("=== LoRa Initialization ===");
  LOG_INFO("📤 Commands: %d, Message: %s, Max retries: %d", 
           lora_ctx.num_commands, lora_ctx.send_message, lora_ctx.max_retry_count);
           
  // SD 카드 로깅 설정 (간단한 방식)
  extern int g_sd_initialization_result; // main()에서 설정된 SD 결과
  if (g_sd_initialization_result == SDSTORAGE_OK) {
    LOG_INFO("🗂️ LoRa logs will be saved to SD card: lora_logs/");
  } else {
    LOG_INFO("📺 LoRa logs will be displayed on terminal only (SD not available)");
  }
  
  // LoRa 로깅 모드 설정 - 초기화 단계에서는 터미널만 사용
  if (g_sd_initialization_result == SDSTORAGE_OK) {
    LOGGER_SetMode(LOGGER_MODE_DUAL);  // 터미널 + SD 동시 출력
    LOGGER_SetFilterLevel(LOG_LEVEL_INFO);  // 터미널에서 모든 로그 확인 가능
    LOGGER_SetSDFilterLevel(LOG_LEVEL_WARN);  // SD 카드에는 WARN 이상만 저장
    LOGGER_EnableSDLogging(false);  // 초기화 완료 전까지 SD 로깅 비활성화
    LOG_WARN("✅ LoRa logging mode: DUAL (Terminal + SD), SD logging will start from JOIN attempts");
  } else {
    LOGGER_SetMode(LOGGER_MODE_TERMINAL_ONLY);
    LOGGER_SetFilterLevel(LOG_LEVEL_INFO);
    LOG_INFO("📺 LoRa logging mode: Terminal only");
  }
  
  // LoRa 프로세스 루프 (초기화 → JOIN → 주기적 전송)
  LOG_INFO("📤 [TX_TASK] Starting LoRa process loop...");
  
  for(;;)
  {
    // 수신된 응답이 있으면 LoraStarter에 전달
    const char* rx_data = NULL;
    if (lora_new_response) {
      rx_data = lora_rx_response;
      lora_new_response = false; // 플래그 클리어
      // 응답 처리 - 로그는 ResponseHandler에서 이미 출력됨
    }
    
    // LoraStarter 프로세스 실행
    LoraStarter_Process(&lora_ctx, rx_data);
    
    // JOIN 성공 후 시간 조회는 LoRa 상태 머신에서 자동 처리됨 (TIMEREQ → LTIME)
    
    // 상태별 처리 간격 및 디버깅 (중요한 상태만)
    static int last_state = -1;
    if (lora_ctx.state != last_state) {
      // JOIN, SEND, ERROR 등 중요한 상태 변경만 로그 출력
      if (lora_ctx.state == LORA_STATE_SEND_JOIN || 
          lora_ctx.state == LORA_STATE_SEND_PERIODIC ||
          lora_ctx.state == LORA_STATE_DONE ||
          lora_ctx.state == LORA_STATE_ERROR) {
        LOG_INFO("[TX_TASK] ⚙️ LoRa State: %d, cmd_index: %d/%d", 
                  lora_ctx.state, lora_ctx.cmd_index, lora_ctx.num_commands);
      }
      last_state = lora_ctx.state;
    }
    
    switch(lora_ctx.state) {
      case LORA_STATE_INIT:
        osDelay(500); // 초기화 상태는 빠르게
        break;
      case LORA_STATE_SEND_CMD:
        LOG_INFO("[TX_TASK] 📤 Sending command %d/%d", 
                lora_ctx.cmd_index + 1, lora_ctx.num_commands);
        osDelay(1000); // 명령어 전송 후 1초 대기
        break;
      case LORA_STATE_WAIT_OK:
        // OK 응답 대기 중 - 조용히 대기
        osDelay(2000); // OK 응답 대기 중 2초 간격
        break;
      case LORA_STATE_SEND_JOIN:
        // JOIN 시도 시작 - SD 로깅 활성화 (영구적)
        if (g_sd_initialization_result == SDSTORAGE_OK && !LOGGER_IsSDLoggingEnabled()) {
          LOGGER_EnableSDLogging(true);
          LOG_WARN("🗂️ SD logging enabled from JOIN attempts (WARN+ levels only)");
        }
        osDelay(2000); // JOIN 명령어 전송 후 2초 대기
        break;
      case LORA_STATE_WAIT_JOIN_OK:
        // JOIN 성공 확인 시 SD 로깅 영구 활성화 보장
        if (g_sd_initialization_result == SDSTORAGE_OK && !LOGGER_IsSDLoggingEnabled()) {
          LOGGER_EnableSDLogging(true);
          LOG_WARN("🗂️ SD logging permanently enabled after JOIN success");
        }
        osDelay(3000); // JOIN 응답 대기 중 3초 간격
        break;
      case LORA_STATE_SEND_TIMEREQ:
        osDelay(1000); // TIMEREQ 명령어 전송 후 1초 대기
        break;
      case LORA_STATE_SEND_LTIME:
        osDelay(1000); // LTIME 명령어 전송 후 1초 대기
        break;
      case LORA_STATE_SEND_PERIODIC:
        // 주기적 SEND 시 SD 로깅 상태 확인 및 활성화
        if (g_sd_initialization_result == SDSTORAGE_OK && !LOGGER_IsSDLoggingEnabled()) {
          LOGGER_EnableSDLogging(true);
          LOG_WARN("🗂️ SD logging re-enabled for periodic SEND");
        }
        osDelay(2000); // SEND 명령어 전송 후 2초 대기
        break;
      case LORA_STATE_WAIT_TIMEREQ_OK:
      case LORA_STATE_WAIT_LTIME_RESPONSE:
      case LORA_STATE_WAIT_SEND_RESPONSE:
        osDelay(3000); // 응답 대기 중 3초 간격
        break;
      case LORA_STATE_WAIT_TIME_SYNC:
        osDelay(1000); // 시간 동기화 대기 중 1초 간격으로 체크
        break;
      case LORA_STATE_WAIT_SEND_INTERVAL:
        // 주기적 전송 대기 중 - 로그 출력 없이 조용히 대기
        osDelay(5000); // 주기적 전송 대기 중 5초 간격으로 체크
        break;
      case LORA_STATE_JOIN_RETRY:
        osDelay(5000); // 재시도 대기 5초
        break;
      case LORA_STATE_DONE:
      case LORA_STATE_ERROR:
        LOG_INFO("📤 [TX_TASK] LoRa process completed with state: %s", 
                lora_ctx.state == LORA_STATE_DONE ? "DONE" : "ERROR");
        goto idle_loop;
      default:
        osDelay(1000);
        break;
    }
  }

idle_loop:
  /* Infinite idle loop */
  LOG_INFO("📤 [TX_TASK] Entering idle mode...");
  uint32_t idle_counter = 0;
  
  for(;;)
  {
    // 30초마다 idle 상태 표시
    osDelay(30000);
    idle_counter++;
    LOG_INFO("📤 [TX_TASK] Idle mode: %lu minutes elapsed", idle_counter / 2);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartSDLoggingTask */
/**
  * @brief  Function implementing the sdLoggingTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSDLoggingTask */
void StartSDLoggingTask(void const * argument)
{
  /* USER CODE BEGIN StartSDLoggingTask */
  LOG_INFO("=== SD Logging Task Started ===");
  
  // 시스템 안정화 대기 (다른 태스크들 먼저 시작)
  osDelay(3000);
  
  // SD 초기화 시도 (이미 정상이면 스킵)
  bool sd_init_needed = !SDStorage_IsReady();
  int init_result = SDSTORAGE_OK; // 기본값: 성공
  
  if (sd_init_needed) {
    LOG_INFO("[SD_TASK] 🔄 Attempting SD card initialization...");
    
    // 단계별 안전한 SD 초기화
    int init_attempts = 0;
    const int MAX_INIT_ATTEMPTS = 3;
    
    for (init_attempts = 0; init_attempts < MAX_INIT_ATTEMPTS; init_attempts++) {
    LOG_INFO("[SD_TASK] Initialization attempt %d/%d", init_attempts + 1, MAX_INIT_ATTEMPTS);
    
    // SDStorage_Init을 타임아웃과 함께 호출
    uint32_t init_start_time = HAL_GetTick();
    const uint32_t INIT_TIMEOUT_MS = 10000;  // 10초 타임아웃
    
    // TODO: 실제로는 별도 태스크에서 SDStorage_Init 호출하고 여기서는 폴링
    // 현재는 간단히 직접 호출하되 타임아웃 체크
    init_result = SDStorage_Init();
    uint32_t init_duration = HAL_GetTick() - init_start_time;
    
    LOG_INFO("[SD_TASK] Init attempt %d took %lu ms, result: %d", 
             init_attempts + 1, init_duration, init_result);
    
    if (init_result == SDSTORAGE_OK) {
      LOG_INFO("[SD_TASK] ✅ SD initialization successful!");
      g_sd_initialization_result = SDSTORAGE_OK;
      g_sd_logging_active = true;
      break;
    } else {
      LOG_WARN("[SD_TASK] ⚠️ SD init attempt %d failed (code: %d)", 
               init_attempts + 1, init_result);
      
      if (init_attempts < MAX_INIT_ATTEMPTS - 1) {
        LOG_INFO("[SD_TASK] Waiting 5 seconds before retry...");
        osDelay(5000);
      }
    }
    } // for loop 종료
  } else {
    // 이미 SD가 준비된 경우
    LOG_INFO("[SD_TASK] 📝 SD card already ready, skipping initialization");
    g_sd_initialization_result = SDSTORAGE_OK;
    g_sd_logging_active = true;
  }
  
  // 초기화 결과에 따른 후속 처리
  if (init_result != SDSTORAGE_OK) {
    LOG_ERROR("[SD_TASK] ❌ All SD initialization attempts failed");
    LOG_INFO("[SD_TASK] Continuing with terminal-only logging");
    
    // SD 실패해도 태스크는 계속 실행 (나중에 재시도 가능)
    for(;;) {
      osDelay(60000);  // 1분마다 재시도 체크 (향후 확장)
    }
  }
  
  LOG_INFO("[SD_TASK] 🗂️ SD logging queue processing started");
  
  // SD 로그 큐 처리 메인 루프
  for(;;)
  {
    SDLogEntry_t log_entry;
    osEvent event = osMessageGet(sdLogQueueHandle, 1000);  // 1초 타임아웃
    
    if (event.status == osEventMessage) {
      // 큐에서 로그 엔트리 수신
      log_entry = *((SDLogEntry_t*)event.value.p);
      
      // SD에 안전하게 쓰기 (타임아웃 포함)
      uint32_t write_start = HAL_GetTick();
      int write_result = SDStorage_WriteLog(log_entry.message, log_entry.length);
      uint32_t write_duration = HAL_GetTick() - write_start;
      
      if (write_result != SDSTORAGE_OK) {
        // SD 쓰기 실패 - 터미널에만 에러 출력 (무한루프 방지)
        printf("[SD_TASK] Write failed (duration: %lu ms, result: %d)\n", 
               write_duration, write_result);
        
        // SD 쓰기 실패 시 잠시 대기 후 재시도 여부 결정
        osDelay(1000);
      }
    }
    
    // 주기적으로 SD 상태 체크 (1분마다)
    static uint32_t status_check_counter = 0;
    status_check_counter++;
    if (status_check_counter % 60 == 0) {  // 60초마다
      if (SDStorage_IsReady()) {
        // SD 상태 정상
      } else {
        // SD 상태 이상 - 재초기화 시도 (향후 확장)
        LOG_WARN("[SD_TASK] SD card appears disconnected - monitoring");
      }
    }
    
    osDelay(50);  // CPU 부하 방지
  }
  /* USER CODE END StartSDLoggingTask */
}

/* USER CODE BEGIN Header_StartReceiveTask */
/**
  * @brief  Function implementing the receiveTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartReceiveTask */
void StartReceiveTask(void const * argument)
{
  /* USER CODE BEGIN StartReceiveTask */
  LOG_INFO("=== DMA-based Receive Task Started ===");
  
  // UART 초기화 대기
  osDelay(2000);
  
  // TDD 모듈들을 사용한 DMA 기반 수신 태스크
  char local_buffer[512];
  int local_bytes_received = 0;
  
  for(;;)
  {
    // TDD UART 모듈을 통한 DMA 기반 수신 체크
    UartStatus status = UART_Receive(local_buffer, sizeof(local_buffer), &local_bytes_received);
    
    // 디버깅용: 수신 상태 체크 (에러 상태일 때만)
    static uint32_t debug_counter = 0;
    debug_counter++;
    if (debug_counter % 1200 == 0 && status != UART_STATUS_TIMEOUT) {  // 1분마다, 타임아웃 제외
      LOG_DEBUG("[RX_TASK] Status check #%lu: status=%d, bytes=%d", 
               debug_counter / 1200, status, local_bytes_received);
    }
    
    if (status == UART_STATUS_OK && local_bytes_received > 0) {
      // 수신 완료 - 간단한 수신 로그 + ResponseHandler 분석
      LOG_INFO("📥 RECV: '%.30s%s' (%d bytes)", 
               local_buffer, 
               (local_bytes_received > 30) ? "..." : "", 
               local_bytes_received);
      
      // 기본적인 응답 타입 체크 (ResponseHandler에서 상세 로그 출력)
      if (strstr(local_buffer, "+EVT:JOINED") != NULL) {
        LOG_WARN("✅ JOIN CONFIRMED - Network joined successfully");
        g_join_success_time = HAL_GetTick();  // JOIN 성공 시간 기록
      } else if (strstr(local_buffer, "RAKwireless") != NULL) {
        LOG_DEBUG("📡 LoRa module boot message (ignored)");
      } else if (ResponseHandler_IsTimeResponse(local_buffer)) {
        // 시간 응답 처리
        ResponseHandler_ParseTimeResponse(local_buffer);
      }
      // 나머지 응답 분석은 아래 필터링 로직에서 한 번만 처리
      
      // 전역 변수에 복사 (다른 태스크에서 사용 가능)
      memcpy(rx_buffer, local_buffer, local_bytes_received);
      rx_bytes_received = local_bytes_received;
      
      // LoRa 상태 머신에 전달할 응답만 필터링
      bool is_lora_command_response = false;
      
      if (is_response_ok(local_buffer)) {
        // OK 응답 - LoRa 명령에 대한 응답
        is_lora_command_response = true;
      } else if (strstr(local_buffer, "+EVT:JOINED") != NULL) {
        // JOIN 성공 응답
        is_lora_command_response = true;
      } else if (ResponseHandler_IsTimeResponse(local_buffer)) {
        // 시간 응답 - LoRa 상태 머신에도 전달해야 함 (상태 전환을 위해)
        is_lora_command_response = true;
      } else if (strstr(local_buffer, "+EVT:") != NULL) {
        // 기타 LoRa 이벤트 응답들
        is_lora_command_response = true;
      } else if (strstr(local_buffer, "RAKwireless") != NULL || strstr(local_buffer, "ORAKwireless") != NULL) {
        // 부트 메시지 - LoRa 상태 머신에 전달하지 않음
        LOG_DEBUG("[RX_TASK] Boot message filtered out from LoRa state machine");
      } else {
        // 기타 응답들 (ERROR, TIMEOUT 등)
        ResponseType response_type = ResponseHandler_ParseSendResponse(local_buffer);
        if (response_type != RESPONSE_UNKNOWN) {
          is_lora_command_response = true;
        }
      }
      
      // LoRa 명령 응답만 전역 변수에 복사
      if (is_lora_command_response) {
        memcpy(lora_rx_response, local_buffer, local_bytes_received);
        lora_rx_response[local_bytes_received] = '\0';
        lora_new_response = true;
        LOG_DEBUG("[RX_TASK] LoRa response forwarded to state machine: %.20s...", local_buffer);
      }
      
      // 버퍼 클리어
      memset(local_buffer, 0, sizeof(local_buffer));
      local_bytes_received = 0;
    }
    
    // DMA 기반이므로 긴 지연으로 CPU 사용률 감소
    osDelay(50);  // 50ms 지연 (DMA가 백그라운드에서 처리하므로 빠른 폴링 불필요)
  }
  /* USER CODE END StartReceiveTask */
}



/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @brief DMA Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration - USART6_RX */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  
  /* USART6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART6_IRQn);
}

/**
  * @brief DMA2 Stream1 DMA configuration for USART6 RX
  * @param None
  * @retval None
  */
void MX_USART6_DMA_Init(void)
{
  // DMA 이미 초기화되었는지 체크
  if (hdma_usart6_rx.Instance != NULL) {
    return; // 이미 초기화됨
  }
  
  /* Configure DMA for USART6 RX */
  hdma_usart6_rx.Instance = DMA2_Stream1;
  hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;
  hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_usart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart6_rx.Init.Mode = DMA_NORMAL;    // 일반 모드로 변경
  hdma_usart6_rx.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_usart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  
  HAL_StatusTypeDef dma_result = HAL_DMA_Init(&hdma_usart6_rx);
  if (dma_result != HAL_OK)
  {
    // 에러 처리하되 Error_Handler() 호출하지 않음 (시스템 중단 방지)
    hdma_usart6_rx.Instance = NULL; // 실패 표시
    return;
  }

  /* Associate the initialized DMA handle to the UART handle */
  __HAL_LINKDMA(&huart6, hdmarx, hdma_usart6_rx);
}
