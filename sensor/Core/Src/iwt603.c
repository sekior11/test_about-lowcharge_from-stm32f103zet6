#include "iwt603.h"
#include <string.h>

#define IWT603_FRAME_SIZE 11U
#define IWT603_HEADER     0x55U

static UART_HandleTypeDef *s_huart;
static uint8_t s_frame[IWT603_FRAME_SIZE];
static uint8_t s_index;
static IWT603_Data_t s_data;

static void IWT603_StartFrame(uint8_t byte)//开始帧
{
  s_frame[0] = byte;
  s_index = 1U;
}

static int16_t IWT603_ToInt16(uint8_t low, uint8_t high)//字节拼接
{
  return (int16_t)(((uint16_t)high << 8U) | low);
}

static uint8_t IWT603_ChecksumValid(const uint8_t *frame)//校验和校验
{
  uint8_t i;
  uint8_t sum = 0U;
  for (i = 0U; i < (IWT603_FRAME_SIZE - 1U); i++)
  {
    sum = (uint8_t)(sum + frame[i]);
  }
  return (sum == frame[IWT603_FRAME_SIZE - 1U]) ? 1U : 0U;
}

static void IWT603_ParseFrame(const uint8_t *frame)
{
  int16_t x = IWT603_ToInt16(frame[2], frame[3]);//加速度x轴
  int16_t y = IWT603_ToInt16(frame[4], frame[5]);//加速度y轴
  int16_t z = IWT603_ToInt16(frame[6], frame[7]);//加速度z轴

  switch (frame[1])
  {
    case 0x51U:
      s_data.acc_g[0] = ((float)x / 32768.0f) * 16.0f;
      s_data.acc_g[1] = ((float)y / 32768.0f) * 16.0f;
      s_data.acc_g[2] = ((float)z / 32768.0f) * 16.0f;
      s_data.temperature_c = (float)IWT603_ToInt16(frame[8], frame[9]) / 100.0f;
      break;
    case 0x52U:
      s_data.gyro_dps[0] = ((float)x / 32768.0f) * 2000.0f;
      s_data.gyro_dps[1] = ((float)y / 32768.0f) * 2000.0f;
      s_data.gyro_dps[2] = ((float)z / 32768.0f) * 2000.0f;
      break;
    case 0x53U:
      s_data.angle_deg[0] = ((float)x / 32768.0f) * 180.0f;
      s_data.angle_deg[1] = ((float)y / 32768.0f) * 180.0f;
      s_data.angle_deg[2] = ((float)z / 32768.0f) * 180.0f;
      break;
    default:
      return;
  }
  s_data.update_tick = HAL_GetTick();
  s_data.valid = 1U;
}

void IWT603_Init(UART_HandleTypeDef *huart)
{
  s_huart = huart;
  s_index = 0U;
  (void)memset(&s_data, 0, sizeof(s_data));
}

void IWT603_RxByte(uint8_t byte)
{
  if (s_index == 0U)
  {
    if (byte == IWT603_HEADER)
    {
      IWT603_StartFrame(byte);
    }
    return;
  }

  if (s_index == 1U)
  {
    if ((byte < 0x50U) || (byte > 0x5AU))
    {
      if (byte == IWT603_HEADER)
      {
        IWT603_StartFrame(byte);
      }
      else
      {
        s_index = 0U;
      }
      return;
    }
  }

  s_frame[s_index++] = byte;
  if (s_index >= IWT603_FRAME_SIZE)
  {
    if (IWT603_ChecksumValid(s_frame) != 0U)
    {
      IWT603_ParseFrame(s_frame);
      s_index = 0U;
    }
    else if (byte == IWT603_HEADER)
    {
      IWT603_StartFrame(byte);
    }
    else
    {
      s_index = 0U;
    }
  }
}

uint8_t IWT603_GetLatest(IWT603_Data_t *data)
{
  if ((data == NULL) || (s_data.valid == 0U))
  {
    return 0U;
  }
  *data = s_data;
  return 1U;
}

HAL_StatusTypeDef IWT603_RequestRegisters(uint8_t start_register)
{
  uint8_t command[5] = {0xFFU, 0xAAU, 0x27U, start_register, 0x00U};
  if (s_huart == NULL)
  {
    return HAL_ERROR;
  }
  return HAL_UART_Transmit(s_huart, command, sizeof(command), 100U);
}
