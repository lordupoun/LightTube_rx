/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADC_BATTERY_Pin GPIO_PIN_1
#define ADC_BATTERY_GPIO_Port GPIOA
#define LED_GREEN_Pin GPIO_PIN_0
#define LED_GREEN_GPIO_Port GPIOB
#define LED_RED_Pin GPIO_PIN_1
#define LED_RED_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_2
#define LED_GPIO_Port GPIOB
#define SPI2_ShutDN_Pin GPIO_PIN_10
#define SPI2_ShutDN_GPIO_Port GPIOB
#define SPI2_IRQ_Pin GPIO_PIN_11
#define SPI2_IRQ_GPIO_Port GPIOB
#define SPI2_IRQ_EXTI_IRQn EXTI15_10_IRQn
#define SPI2_GPIO_NSS_Pin GPIO_PIN_12
#define SPI2_GPIO_NSS_GPIO_Port GPIOB
#define TIM_ARGB_Pin GPIO_PIN_8
#define TIM_ARGB_GPIO_Port GPIOA
#define UART_DEBUG_TX_Pin GPIO_PIN_9
#define UART_DEBUG_TX_GPIO_Port GPIOA
#define UART_DEBUG_RX_Pin GPIO_PIN_10
#define UART_DEBUG_RX_GPIO_Port GPIOA
#define DIP4_Pin GPIO_PIN_11
#define DIP4_GPIO_Port GPIOA
#define DIP3_Pin GPIO_PIN_12
#define DIP3_GPIO_Port GPIOA
#define DIP2_Pin GPIO_PIN_15
#define DIP2_GPIO_Port GPIOA
#define DIP1_Pin GPIO_PIN_3
#define DIP1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
