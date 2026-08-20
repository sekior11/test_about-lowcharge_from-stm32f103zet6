#include "data_receiver.h"

static uint8_t s_buffer[DATA_RECEIVER_BUFFER_SIZE];
static volatile uint16_t s_write_index;
static volatile uint16_t s_read_index;
static volatile uint32_t s_overflow_count;

void DataReceiver_Init(UART_HandleTypeDef *huart)
{
  (void)huart;
  s_write_index = 0U;
  s_read_index = 0U;
  s_overflow_count = 0U;
}

void DataReceiver_RxByte(uint8_t byte)
{
  uint16_t next_index = (uint16_t)((s_write_index + 1U) % DATA_RECEIVER_BUFFER_SIZE);

  if (next_index == s_read_index)
  {
    s_overflow_count++;
    return;
  }

  s_buffer[s_write_index] = byte;
  s_write_index = next_index;
}

uint16_t DataReceiver_Available(void)
{
  if (s_write_index >= s_read_index)
  {
    return (uint16_t)(s_write_index - s_read_index);
  }
  return (uint16_t)(DATA_RECEIVER_BUFFER_SIZE - s_read_index + s_write_index);
}

uint8_t DataReceiver_Read(uint8_t *byte)
{
  if ((byte == NULL) || (s_read_index == s_write_index))
  {
    return 0U;
  }

  *byte = s_buffer[s_read_index];
  s_read_index = (uint16_t)((s_read_index + 1U) % DATA_RECEIVER_BUFFER_SIZE);
  return 1U;
}

uint32_t DataReceiver_OverflowCount(void)
{
  return s_overflow_count;
}
