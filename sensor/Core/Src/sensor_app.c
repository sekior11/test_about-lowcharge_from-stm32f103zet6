#include "sensor_app.h"
#include <stdio.h>
#include "main.h"
#include "usart.h"
#include "fa66.h"

#define SENSOR_APP_REPORT_PERIOD_MS       200U
#define SENSOR_APP_REPORT_BUFFER_SIZE     256U
#define IWT603_DMA_RX_BUFFER_SIZE         128U
#define IWT603_ALARM_THRESHOLD_G          1.5f

static SensorApp_Data_t s_sensor_data;
static uint8_t s_iwt603_dma_rx_buffer[IWT603_DMA_RX_BUFFER_SIZE];
static uint8_t s_report_buffer[SENSOR_APP_REPORT_BUFFER_SIZE];
static uint32_t s_last_report_tick;
static uint32_t s_last_iwt603_tick;
static volatile uint32_t s_iwt603_rx_bytes;
static volatile uint32_t s_iwt603_uart_errors;
static uint32_t s_iwt603_valid_updates;

/* 虚拟时间基准: STOP 下 SysTick 冻结, 改用 RTC 计数器(每 tick 200ms)推导 ms */
static uint32_t s_virtual_ms;
static uint32_t s_last_rtc_cnt;
static uint8_t  s_rtc_base_inited;   /* 0=尚未建立首次基准 */

/* FA66 报警会话状态机: 超阈值->继电器吸合->FA66 上电->读毛重; 滞回->断电 */
static FA66_Data_t s_fa66_data;
static uint8_t s_fa66_relay_on;       /* 继电器(FA66 电源)是否吸合 */
static uint32_t s_fa66_relay_on_tick; /* 继电器闭合时刻 */
static uint8_t s_fa66_stabilized;     /* FA66 是否已稳定可读取 */
static uint32_t s_fa66_last_read_tick;
static uint32_t s_alarm_low_since;    /* 阈值低于起算时刻(滞回) */

static float SensorApp_Abs(float value)//绝对值
{
  return (value < 0.0f) ? -value : value;
}

static float SensorApp_Sqrt(float value)//平方根
{
  float estimate;
  uint8_t i;

  if (value <= 0.0f)
  {
    return 0.0f;
  }

  estimate = (value > 1.0f) ? value : 1.0f;
  for (i = 0U; i < 8U; i++)
  {
    estimate = 0.5f * (estimate + (value / estimate));
  }
  return estimate;
}

uint32_t SensorApp_GetVirtualMs(void)
{
  /* STOP 下 SysTick 冻结, 时间基准改用 RTC 计数器(预分频 5Hz, 每 tick 200ms) */
  uint32_t cnt = (((uint32_t)RTC->CNTH << 16U) | RTC->CNTL);
  if (s_rtc_base_inited == 0U)
  {
    s_last_rtc_cnt = cnt;          /* 首次建立基准, 不累加 */
    s_rtc_base_inited = 1U;
  }
  else if (cnt != s_last_rtc_cnt)
  {
    s_virtual_ms += (cnt - s_last_rtc_cnt) * 200U;   /* 无符号减法自动处理回绕 */
    s_last_rtc_cnt = cnt;
  }
  return s_virtual_ms;
}

static void SensorApp_ProcessIwt603Bytes(const uint8_t *data, uint16_t length)
{
  uint16_t i;

  for (i = 0U; i < length; i++)
  {
    IWT603_RxByte(data[i]);
  }
  s_iwt603_rx_bytes += length;
}

static void SensorApp_StartIwt603Dma(void)//启动DMA接收
{
  if (HAL_UART_Receive_DMA(&huart3,
                           s_iwt603_dma_rx_buffer,
                           IWT603_DMA_RX_BUFFER_SIZE) != HAL_OK)
  {
    Error_Handler();
  }
}

void SensorApp_Init(void)//初始化传感器应用
{
  IWT603_Init(&huart3);
  s_virtual_ms = 0U;
  s_last_rtc_cnt = 0U;
  s_rtc_base_inited = 0U;
  s_last_report_tick = 0U;
  s_last_iwt603_tick = 0U;
  s_iwt603_rx_bytes = 0U;
  s_iwt603_uart_errors = 0U;
  s_iwt603_valid_updates = 0U;
  /* 修漏洞: 初始未报警, 主动关闭 USART2 时钟(原逻辑走不到), 省常态漏电 */
  __HAL_RCC_USART2_CLK_DISABLE();
  SensorApp_StartIwt603Dma();
}

/* FA66 报警会话控制: 阈值超限 -> 继电器吸合(PB5 高) -> FA66 上电;
   上电稳定后周期读取双通道毛重并经 USART1 上报; 阈值持续低于后滞回断电。
   仅在报警会话期间开启 USART2 时钟, 会话结束关闭以省电。 */
static void SensorApp_ControlFa66(uint8_t alarm_active)
{
  uint32_t now = SensorApp_GetVirtualMs();

  if (alarm_active != 0U)
  {
    s_alarm_low_since = 0U;
    if (s_fa66_relay_on == 0U)
    {
      HAL_GPIO_WritePin(ALARM_OUT_GPIO_Port, ALARM_OUT_Pin, GPIO_PIN_SET);
      s_fa66_relay_on = 1U;
      s_fa66_relay_on_tick = now;
      s_fa66_stabilized = 0U;
      s_fa66_last_read_tick = now;
      __HAL_RCC_USART2_CLK_ENABLE();      /* 仅在会话期间开启 FA66 串口时钟 */
    }

    if (s_fa66_stabilized == 0U)
    {
      if ((now - s_fa66_relay_on_tick) >= FA66_POWER_STABILIZE_MS)
      {
        s_fa66_stabilized = 1U;
      }
    }
    else if ((now - s_fa66_last_read_tick) >= FA66_READ_PERIOD_MS)
    {
      s_fa66_last_read_tick = now;
      if (FA66_ReadGrossWeights(&huart2, RS485_DE_GPIO_Port, RS485_DE_Pin,
                                &s_fa66_data) == MODBUS_RTU_OK)
      {
        char fa66_buf[64];
        int len = snprintf(fa66_buf, sizeof(fa66_buf),
                           "FA66,%ld,%ld\r\n",
                           (long)s_fa66_data.gross_weight[0],
                           (long)s_fa66_data.gross_weight[1]);
        if ((len > 0) && ((size_t)len < sizeof(fa66_buf)))
        {
          (void)HAL_UART_Transmit(&huart1, (uint8_t *)fa66_buf,
                                  (uint16_t)len, 100U);
        }
      }
    }
  }
  else
  {
    if (s_fa66_relay_on != 0U)
    {
      if (s_alarm_low_since == 0U)
      {
        s_alarm_low_since = now;
      }
      if ((now - s_alarm_low_since) >= FA66_ALARM_CLEAR_HOLD_MS)
      {
        HAL_GPIO_WritePin(ALARM_OUT_GPIO_Port, ALARM_OUT_Pin, GPIO_PIN_RESET);
        s_fa66_relay_on = 0U;
        s_fa66_stabilized = 0U;
        __HAL_RCC_USART2_CLK_DISABLE();   /* 关闭 FA66 串口时钟省电 */
      }
    }
  }
}

void SensorApp_Process(void)
{
  IWT603_Data_t latest;
  uint32_t now;

  if (IWT603_GetLatest(&latest) != 0U)
  {
    if (latest.update_tick != s_last_iwt603_tick)
    {
      s_sensor_data.attitude = latest;
      s_last_iwt603_tick = latest.update_tick;
      s_iwt603_valid_updates++;
    }
  }

  now = SensorApp_GetVirtualMs();
  if ((now - s_last_report_tick) >= SENSOR_APP_REPORT_PERIOD_MS)
  {
    int length;
    float acc_magnitude;
    float acc_deviation;
    uint8_t status;

    s_last_report_tick = now;
    acc_magnitude = SensorApp_Sqrt(
        (s_sensor_data.attitude.acc_g[0] * s_sensor_data.attitude.acc_g[0]) +
        (s_sensor_data.attitude.acc_g[1] * s_sensor_data.attitude.acc_g[1]) +
        (s_sensor_data.attitude.acc_g[2] * s_sensor_data.attitude.acc_g[2]));
    acc_deviation = SensorApp_Abs(acc_magnitude - 1.0f);
    status = (acc_magnitude > IWT603_ALARM_THRESHOLD_G) ? 1U : 0U;
    SensorApp_ControlFa66(status);

    length = snprintf((char *)s_report_buffer, sizeof(s_report_buffer),
                      "%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%lu,%lu\r\n",
                      (unsigned long)now,
                      (double)s_sensor_data.attitude.acc_g[0],
                      (double)s_sensor_data.attitude.acc_g[1],
                      (double)s_sensor_data.attitude.acc_g[2],
                      (double)acc_magnitude,
                      (double)acc_deviation,
                      (double)s_sensor_data.attitude.gyro_dps[0],
                      (double)s_sensor_data.attitude.gyro_dps[1],
                      (double)s_sensor_data.attitude.gyro_dps[2],
                      (double)s_sensor_data.attitude.angle_deg[0],
                      (double)s_sensor_data.attitude.angle_deg[1],
                      (double)s_sensor_data.attitude.angle_deg[2],
                      (unsigned int)status,
                      (unsigned long)s_iwt603_rx_bytes,
                      (unsigned long)s_iwt603_uart_errors);
    if ((length > 0) && ((size_t)length < sizeof(s_report_buffer)))
    {
      (void)HAL_UART_Transmit(&huart1, s_report_buffer, (uint16_t)length, 100U);
    }
  }
}

const SensorApp_Data_t *SensorApp_GetData(void)
{
  return &s_sensor_data;
}

uint8_t SensorApp_IsAlarmSessionActive(void)
{
  return s_fa66_relay_on;
}

void SensorApp_RestartIwt603Dma(void)
{
  (void)HAL_UART_DMAStop(&huart3);
  /* STOP 唤醒后重武装 DMA 前先清一次溢出标志: 唤醒窗口内(恢复时钟到重启动之间
     约 1~2ms) IWT603(921600)持续广播, 字节会填满 USART 移位寄存器触发 ORE,
     若不清除会导致 uart_errors 每周期累加。清掉挂起的 ORE 可消除该计数。 */
  __HAL_UART_CLEAR_OREFLAG(&huart3);
  SensorApp_StartIwt603Dma();
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    SensorApp_ProcessIwt603Bytes(s_iwt603_dma_rx_buffer,
                                 IWT603_DMA_RX_BUFFER_SIZE / 2U);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    SensorApp_ProcessIwt603Bytes(&s_iwt603_dma_rx_buffer[IWT603_DMA_RX_BUFFER_SIZE / 2U],
                                 IWT603_DMA_RX_BUFFER_SIZE / 2U);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    s_iwt603_uart_errors++;
    (void)HAL_UART_DMAStop(&huart3);
    __HAL_UART_CLEAR_OREFLAG(&huart3);
    SensorApp_StartIwt603Dma();
  }
}
