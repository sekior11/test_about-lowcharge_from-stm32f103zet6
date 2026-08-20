#include "fa66.h"

ModbusRtu_Status_t FA66_ReadGrossWeights(UART_HandleTypeDef *huart,
                                         GPIO_TypeDef *de_port,
                                         uint16_t de_pin,
                                         FA66_Data_t *data)
{
  uint16_t registers[FA66_CHANNEL_COUNT * 2U];
  uint8_t channel;
  ModbusRtu_Status_t status;

  if (data == NULL)
  {
    return MODBUS_RTU_ERR_ARGUMENT;
  }

  status = ModbusRtu_ReadHoldingRegisters(huart, de_port, de_pin,
                                          FA66_SLAVE_ADDRESS,
                                          FA66_GROSS_WEIGHT_REG0,
                                          FA66_CHANNEL_COUNT * 2U,
                                          registers, 200U);
  data->status = status;
  if (status != MODBUS_RTU_OK)
  {
    return status;
  }

  for (channel = 0U; channel < FA66_CHANNEL_COUNT; channel++)
  {
    data->gross_weight[channel] = (int32_t)(((uint32_t)registers[channel * 2U] << 16U) |
                                             registers[channel * 2U + 1U]);
  }
  data->update_tick = HAL_GetTick();
  return MODBUS_RTU_OK;
}
