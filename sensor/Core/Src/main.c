/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "sensor_app.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void MX_RTC_Init(void);
void RTC_Alarm_SetNext(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  MX_RTC_Init();                            /* 启用 RTC + 5Hz(200ms) 周期唤醒 */
  SensorApp_Init();

  /* 功耗优化: 释放 JTAG(保留 SWD 调试), 将 PA15/PB3/PB4 设为模拟输入降低漏电 */
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
  {
    GPIO_InitTypeDef gp = {0};
    gp.Mode = GPIO_MODE_ANALOG;
    gp.Pull = GPIO_NOPULL;
    gp.Pin = GPIO_PIN_15;                 /* PA15 (JTAG JTDI) */
    HAL_GPIO_Init(GPIOA, &gp);
    gp.Pin = GPIO_PIN_3 | GPIO_PIN_4;     /* PB3(JTDO), PB4(JNTRST) */
    HAL_GPIO_Init(GPIOB, &gp);
  }

  /* 调试自检: 上电先发一条固定串, 确认程序已进入 main 循环且 USART1 输出链路正常 */
  (void)HAL_UART_Transmit(&huart1, (uint8_t *)"BOOT OK\r\n", 9U, 100U);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    SensorApp_Process();

    if (SensorApp_IsAlarmSessionActive() != 0U)
    {
      /* 报警会话期(FA66 上电约5s): 保持 Sleep 连续处理, 不进 STOP,
         以保证 FA66/USART2/USART3 时钟始终可用 */
      __WFI();
    }
    else
    {
      /* 常态: 进 STOP, 由 RTC Alarm(EXTI17) 每 200ms 唤醒 */
      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
      /* 清 RTC Alarm 标志 + EXTI17 挂起位(裸寄存器, 本项目无 hrtc 句柄):
         否则标志一直挂起, EXTI17 立即再触发, 进 STOP 等于 busy 唤醒不省电 */
      while ((RTC->CRL & RTC_CRL_RTOFF) == 0U) {}          /* 等 RTC 写不忙 */
      RTC->CRL &= (uint16_t)~RTC_CRL_ALRF;                 /* 清报警标志 */
      EXTI->PR = (1U << 17U);                              /* 清 EXTI17 挂起 */
      RTC_Alarm_SetNext();                          /* 重设下一次 200ms 闹钟 */
      HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
      /* ---- 唤醒后从此处继续: 必须先恢复时钟树与外设 ---- */
      SystemClock_Config();                 /* 恢复 72MHz (否则串口波特率错乱) */
      SensorApp_RestartIwt603Dma();         /* 重启 USART3+DMA 接收 */
    }
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
}

/* USER CODE BEGIN 4 */
void RTC_Alarm_SetNext(void)
{
  /* 设置下一次 RTC Alarm = 当前计数器 +1 (F1 RTC 每 tick 200ms 一次) */
  uint32_t cnt = (((uint32_t)RTC->CNTH << 16U) | RTC->CNTL) + 1U;
  RTC->CRL |= RTC_CRL_CNF;                  /* 进入配置模式 */
  RTC->ALRL = (uint16_t)(cnt & 0xFFFFU);
  RTC->ALRH = (uint16_t)(cnt >> 16U);
  RTC->CRL &= ~RTC_CRL_CNF;                 /* 退出配置模式 */
  while ((RTC->CRL & RTC_CRL_RTOFF) == 0U)  /* 等待写完成 */
  {
  }
}

void MX_RTC_Init(void)
{
  /* 1) 使能电源/备份域访问/LSI 时钟 */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_BKP_CLK_ENABLE();
  __HAL_RCC_LSI_ENABLE();
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET)
  {
  }

  /* 2) 选择 LSI 作为 RTC 时钟并开启 (写备份域 BDCR) */
  RCC->BDCR &= ~((uint32_t)0x00000300U);    /* 清 RTCSEL */
  RCC->BDCR |=  (uint32_t)0x00000200U;      /* 选 LSI */
  RCC->BDCR |=  (uint32_t)0x00008000U;      /* RTCEN (bit15) */

  /* 3) 配置预分频: LSI 40kHz / (7999+1) = 5Hz => 每 tick 200ms */
  while ((RTC->CRL & RTC_CRL_RTOFF) == 0U)
  {
  }
  RTC->CRL |= RTC_CRL_CNF;
  RTC->PRLL = 7999U;
  RTC->PRLH = 0U;
  RTC->CRL &= ~RTC_CRL_CNF;
  while ((RTC->CRL & RTC_CRL_RTOFF) == 0U)
  {
  }

  /* 4) 使能 Alarm 中断 + EXTI 线17 上升沿 + NVIC */
  RTC->CRH |= RTC_CRH_ALRIE;
  EXTI->IMR |= (1U << 17U);
  EXTI->RTSR |= (1U << 17U);
  HAL_NVIC_SetPriority(RTC_IRQn, 0U, 0U);
  HAL_NVIC_EnableIRQ(RTC_IRQn);

  /* 5) 设置首次 Alarm = CNT+1 */
  RTC_Alarm_SetNext();

  /* 清残留报警/挂起标志, 避免上电后首次进 STOP 立即误唤醒 */
  while ((RTC->CRL & RTC_CRL_RTOFF) == 0U) {}
  RTC->CRL &= (uint16_t)~RTC_CRL_ALRF;
  EXTI->PR = (1U << 17U);
}
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
