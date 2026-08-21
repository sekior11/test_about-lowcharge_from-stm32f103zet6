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
#define ALARM_OUT_Pin GPIO_PIN_5
#define ALARM_OUT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define ALARM_OUT_Pin GPIO_PIN_5
#define ALARM_OUT_GPIO_Port GPIOB

/* FA66 RS485 (Modbus RTU) 接口定义 */
#define RS485_DE_Pin        GPIO_PIN_0
#define RS485_DE_GPIO_Port  GPIOB
#ifndef FA66_USART_BAUDRATE
#define FA66_USART_BAUDRATE 9600U
#endif
/* FA66 由继电器供电: 超阈值吸合上电, 稳态后读取, 滞回后断电 */
#ifndef FA66_POWER_STABILIZE_MS
#define FA66_POWER_STABILIZE_MS  2000U
#endif
#ifndef FA66_READ_PERIOD_MS
#define FA66_READ_PERIOD_MS      500U
#endif
#ifndef FA66_ALARM_CLEAR_HOLD_MS
#define FA66_ALARM_CLEAR_HOLD_MS 3000U
#endif
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
