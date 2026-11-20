/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32l1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdio.h"
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
#define High2_check_Pin GPIO_PIN_13
#define High2_check_GPIO_Port GPIOC
#define On_Check_Pin GPIO_PIN_0
#define On_Check_GPIO_Port GPIOA
#define LR_Check_Pin GPIO_PIN_1
#define LR_Check_GPIO_Port GPIOA
#define Width1_Check_Pin GPIO_PIN_4
#define Width1_Check_GPIO_Port GPIOA
#define Width2_Check_Pin GPIO_PIN_5
#define Width2_Check_GPIO_Port GPIOA
#define Width3_Check_Pin GPIO_PIN_6
#define Width3_Check_GPIO_Port GPIOA
#define High1_Check_Pin GPIO_PIN_7
#define High1_Check_GPIO_Port GPIOA
#define Power_Ctrl_Pin GPIO_PIN_0
#define Power_Ctrl_GPIO_Port GPIOB
#define Beep_Ctrl_Pin GPIO_PIN_8
#define Beep_Ctrl_GPIO_Port GPIOA
#define Led_R_Pin GPIO_PIN_15
#define Led_R_GPIO_Port GPIOA
#define Led_L_Pin GPIO_PIN_3
#define Led_L_GPIO_Port GPIOB
#define Lcd_En_Pin GPIO_PIN_4
#define Lcd_En_GPIO_Port GPIOB
#define Mem_Check_Pin GPIO_PIN_5
#define Mem_Check_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
