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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TOUCH_Pin GPIO_PIN_0
#define TOUCH_GPIO_Port GPIOA
#define MFR_RST_Pin GPIO_PIN_1
#define MFR_RST_GPIO_Port GPIOA
#define MFR_MISO_Pin GPIO_PIN_4
#define MFR_MISO_GPIO_Port GPIOA
#define MFR_MOSI_Pin GPIO_PIN_5
#define MFR_MOSI_GPIO_Port GPIOA
#define MFR_SCK_Pin GPIO_PIN_6
#define MFR_SCK_GPIO_Port GPIOA
#define MFR_SDA_Pin GPIO_PIN_7
#define MFR_SDA_GPIO_Port GPIOA
#define SW1_Pin GPIO_PIN_0
#define SW1_GPIO_Port GPIOB
#define SW2_Pin GPIO_PIN_1
#define SW2_GPIO_Port GPIOB
#define IN4_Pin GPIO_PIN_10
#define IN4_GPIO_Port GPIOB
#define IN3_Pin GPIO_PIN_11
#define IN3_GPIO_Port GPIOB
#define OLED_SCK_Pin GPIO_PIN_12
#define OLED_SCK_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_13
#define OLED_SDA_GPIO_Port GPIOB
#define SW3_Pin GPIO_PIN_11
#define SW3_GPIO_Port GPIOA
#define IN2_Pin GPIO_PIN_12
#define IN2_GPIO_Port GPIOA
#define IN1_Pin GPIO_PIN_15
#define IN1_GPIO_Port GPIOA
#define SW4_Pin GPIO_PIN_3
#define SW4_GPIO_Port GPIOB
#define SW5_Pin GPIO_PIN_4
#define SW5_GPIO_Port GPIOB
#define SW6_Pin GPIO_PIN_5
#define SW6_GPIO_Port GPIOB
#define SW7_Pin GPIO_PIN_6
#define SW7_GPIO_Port GPIOB
#define SW8_Pin GPIO_PIN_7
#define SW8_GPIO_Port GPIOB
#define BEEP_Pin GPIO_PIN_8
#define BEEP_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
