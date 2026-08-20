#include "sensor_app.h"
#include <stdio.h>
#include "main.h"
#include "usart.h"

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
  s_last_report_tick = HAL_GetTick();
  s_last_iwt603_tick = 0U;
  s_iwt603_rx_bytes = 0U;
  s_iwt603_uart_errors = 0U;
  s_iwt603_valid_updates = 0U;
  SensorApp_StartIwt603Dma();
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

  now = HAL_GetTick();
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
    HAL_GPIO_WritePin(ALARM_OUT_GPIO_Port, ALARM_OUT_Pin,
                      (status != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    length = snprintf((char *)s_report_buffer, sizeof(s_report_buffer),
                      "%lu,%.4f,%.4f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u\r\n",
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
                      (unsigned int)status);
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
