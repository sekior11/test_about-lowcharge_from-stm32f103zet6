#ifndef __IWT603_H
#define __IWT603_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef struct
{
  float acc_g[3];
  float gyro_dps[3];
  float angle_deg[3];
  float temperature_c;
  uint32_t update_tick;
  uint8_t valid;
} IWT603_Data_t;

void IWT603_Init(UART_HandleTypeDef *huart);
void IWT603_RxByte(uint8_t byte);
uint8_t IWT603_GetLatest(IWT603_Data_t *data);
HAL_StatusTypeDef IWT603_RequestRegisters(uint8_t start_register);

#endif /* __IWT603_H */
