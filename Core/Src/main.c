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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "si4432.h"
#include "effects.h"
#include "colours.h" //ToDo: delete
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEV_ADDR 0x11 //Device number
#define PLD_SIZE 12 //payload size; 32 = max for nrf; //ToDo: HEADER_SIZE+NUM_OF_CHANNELS
//#define RX_NUM 6 //number of receivers
#define HEADER_SIZE 6    //num of bytes for header
#define NUM_OF_CHANNELS   6   //num of bytes for data
#define DATA_START_POS 2 //Position of packet on which DMX data starts
//#define DMXPACKET_SIZE 513 //do not change
//#define DMX_STARTBYTE 0 //defines what startbyte the receiver listens to; 0 default for light control by standard

#define AWAKE_PACKET_LENGTH 2
#define AWAK_PACKET_PERIOD 20 //20ms by default

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static volatile uint32_t step=0;
static volatile uint8_t nextStepFlag = 1;
//static uint8_t dmxRX[514];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static volatile bool rxDoneFlag=false;
static uint8_t rx[PLD_SIZE];
static uint8_t dataPacket[1][6];

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
    	nextStepFlag=1;
    }
}

//---RF EXTI Callback---
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//EXTI for RF module (SI4432)
	//if (GPIO_Pin == GPIO_PIN_11)
	//{
        rxDoneFlag=true; //ToDo: přidat čtecí
	//}
}

/*void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
	//memcpy(dmxPrevPacket, dmxPacket, DMXPACKET_SIZE);
	//memcpy(&dmxPacket[0], &dmxRX[1], DMXPACKET_SIZE);

    HAL_UARTEx_ReceiveToIdle_IT(&huart1, dmxRX, 513); //DMXPACKET_SIZE
    //dmxPacket[3]=255;
    //HAL_Delay(5000);
}*/
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
  HAL_Delay(500);
  //---SI4432---
  SI44_Init(&hspi2, GPIOB, GPIO_PIN_12);
  //Delay used to be here
  SI44_PresetConfig();
  HAL_Delay(5000); //ToDo: zkratit
  SI44_SetAGCMode(0b00100000); //6th bit - sgin
  HAL_Delay(10);
  SI44_SetInterrupts1(0b00000010); //1st bit = CRC error
  HAL_Delay(10);
  SI44_SetInterrupts2(0b00000000);
  HAL_Delay(10);
  SI44_SetTXPower(SI44_TX_POWER_20dBm);    //Set TX power to 11dBm (12.5 mW)
  HAL_Delay(500);


  //---ARGB effects---
  effects_init(&htim2); //before timer?
  //ARGB_Init();
  //ARGB_Clear();
  //ARGB_Clear();
  //ARGB_Show();

  //HAL_TIM_Base_Start_IT(&htim2);

  uint8_t testPacket[64];
  testPacket[0]=255;
  testPacket[1]=0;
  testPacket[2]=0;
  testPacket[3]=0;
  testPacket[4]=0;
  testPacket[5]=0;
  testPacket[6]=0;

  effects_set_effect(82,22); //neošetřenej stav bez efektu?
  //effects_set_effect(87,57);

  //HAL_UARTEx_ReceiveToIdle_IT(&huart1, dmxRX, 513);
  //HAL_UARTEx_ReceiveToIdle_IT(&huart1, dmxRX, 513);
  //ARGB_Clear();
  //ARGB_FillRGB(255, 255, 0);
  //ARGB_Show();

  //SetRXon should be placed as close as possible to the main loop,
  //if not, FIFO buffer can overflow, which resets ipkvalid IRQ to logical 1.
  //That would be caused by Si4432 already running in the background and receiving packets, while program for example waiting in HAL_Delay.
  //(If FIFO overflows, the entire packet cannot be saved into FIFO -> invalid paket received.)
  //That makes the program stuck with no option to start receiving again.
  //ToDo: Fix this
  //BEWARE - after resetting or quick ON/OFF switching FIFO/receiving seems to stay active -> FIFO will be kept full
  uint8_t b; //jen pro reset 0x03 registru
  SI44_Read(0x04, &b, 1); //Přehodit do Read knihovny?
  SI44_Read(0x03, &b, 1);
  rxDoneFlag = 0; //EXTREMELY IMPORTANT! Fake IRQ signal probably caused by Si4432 booting up!
  __HAL_GPIO_EXTI_CLEAR_IT(SPI2_IRQ_Pin);
  SI44_ClearRXFIFO();
  SI44_SetRXon();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  //ARGB_FillRGB(255, 255, 0);
	  //ARGB_Show();
	  /*ARGB_Clear();

	  HAL_Delay(1000);*/
	  //Recieving only applies when data changes - included in transmitter code
	  if(rxDoneFlag==1 || HAL_GPIO_ReadPin(SPI2_IRQ_GPIO_Port, SPI2_IRQ_Pin) == GPIO_PIN_RESET)
	  {
		  rxDoneFlag=0;



		  SI44_ReadPacket(testPacket);
		  //ARGB_SetBrightness(255);
		  //ARGB_Clear();
		  ARGB_FillRGB(testPacket[0], testPacket[1], testPacket[2]);
		  //ARGB_FillRGB(testPacket[1], testPacket[2], testPacket[3]);
		  ARGB_Show();
		  //effects_set_effect(testPacket[1],testPacket[2]);

		  char uartBuf[50];  // dostatečně velký buffer
		  int len = sprintf(uartBuf, "%d %d %d\r\n", testPacket[1], testPacket[2], testPacket[3]);
		  HAL_UART_Transmit_IT(&huart1, (uint8_t*)uartBuf, len);

		  uint8_t b; //jen pro reset 0x03 registru
		  SI44_Read(0x04, &b, 1); //Přehodit do Read knihovny?
		  SI44_Read(0x03, &b, 1); //jinak by uz neaktivoval IRQ
	  }
	  //char uartBuf[50];  // dostatečně velký buffer
	  //int len = sprintf(uartBuf, "%d %d %d\r\n", testPacket[1], testPacket[1], testPacket[3]);
	  //HAL_UART_Transmit_IT(&huart1, (uint8_t*)uartBuf, len);
	  /*ARGB_SetBrightness(255);
	  ARGB_FillRGB(255, 0, 0);
	  ARGB_Show();
	  HAL_Delay(1000);
	  ARGB_SetBrightness(255);
	  	  ARGB_FillRGB(0,255, 0);
	  	  ARGB_Show();
	  	HAL_Delay(1000);
	*/
	  //ToDo: check gamma
	 //static ColourName_t currentColour = PRIMARY_BLUE;
	 //HAL_UART_Receive(&huart1, testPacket, 4, 100);
	 //HAL_Delay(0);
	 /*if(dmxRX[5]==0) ------------------------- VLASTNÍ BARVY!
	 {
		 ARGB_Clear();
		 ARGB_SetBrightness(255);

		 ARGB_FillRGB(dmxRX[1], dmxRX[2], dmxRX[3]);
		 ARGB_FillWhite(dmxRX[4]);
		 ARGB_Show();
	 }
	 else
	 {
		 ARGB_SetBrightness(255);
		 ARGB_FillRGB(
		 		colourTable[dmxRX[5]].r,
		 		colourTable[dmxRX[5]].g,
		 		colourTable[dmxRX[5]].b);
		 	ARGB_FillWhite(colourTable[dmxRX[5]].w);
		 	ARGB_Show();
	 }
	 HAL_Delay(500);*/

	 if(nextStepFlag==1) //---------------- ODKOMENTOVAT!!!
	  {
		  effects_next_step();
		  nextStepFlag=0;
	  } //--------------------------------- ODKOMENTOVAT!!!

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
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
  htim1.Init.Period = 89;
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

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 6399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  huart1.Init.BaudRate = 250000;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_2;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_RED_Pin|LED_GREEN_Pin|LED_Pin|SPI2_ShutDN_Pin
                          |SPI2_GPIO_NSS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_RED_Pin LED_GREEN_Pin LED_Pin SPI2_ShutDN_Pin */
  GPIO_InitStruct.Pin = LED_RED_Pin|LED_GREEN_Pin|LED_Pin|SPI2_ShutDN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_IRQ_Pin */
  GPIO_InitStruct.Pin = SPI2_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SPI2_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_GPIO_NSS_Pin */
  GPIO_InitStruct.Pin = SPI2_GPIO_NSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(SPI2_GPIO_NSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DIP1_Pin DIP2_Pin DIP3_Pin */
  GPIO_InitStruct.Pin = DIP1_Pin|DIP2_Pin|DIP3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : DIP4_Pin */
  GPIO_InitStruct.Pin = DIP4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DIP4_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
