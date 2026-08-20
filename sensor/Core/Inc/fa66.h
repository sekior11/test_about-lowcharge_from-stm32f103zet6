#ifndef __FA66_H
#define __FA66_H

#include "stm32f1xx_hal.h"
#include "modbus_rtu.h"
#include <stdint.h>

#ifndef FA66_SLAVE_ADDRESS
#define FA66_SLAVE_ADDRESS             1U
#endif

/* 40451 maps to zero-based Modbus holding-register address 450 (0x01C2). */
#ifndef FA66_GROSS_WEIGHT_REG0
#define FA66_GROSS_WEIGHT_REG0         450U
#endif

#ifndef FA66_CHANNEL_COUNT
#define FA66_CHANNEL_COUNT             2U
#endif

typedef struct
{
  int32_t gross_weight[FA66_CHANNEL_COUNT];
  uint32_t update_tick;
  ModbusRtu_Status_t status;
} FA66_Data_t;

ModbusRtu_Status_t FA66_ReadGrossWeights(UART_HandleTypeDef *huart,
                                         GPIO_TypeDef *de_port,
                                         uint16_t de_pin,
                                         FA66_Data_t *data);

#endif /* __FA66_H */
