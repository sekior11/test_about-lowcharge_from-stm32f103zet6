#include "modbus_rtu.h"

#define MODBUS_RTU_REQUEST_SIZE 8U

static void ModbusRtu_SetReceive(GPIO_TypeDef *de_port, uint16_t de_pin)
{
  HAL_GPIO_WritePin(de_port, de_pin, GPIO_PIN_RESET);
}

static void ModbusRtu_SetTransmit(GPIO_TypeDef *de_port, uint16_t de_pin)
{
  HAL_GPIO_WritePin(de_port, de_pin, GPIO_PIN_SET);
}

uint16_t ModbusRtu_Crc16(const uint8_t *data, uint16_t length)
{
  uint16_t crc = 0xFFFFU;
  uint16_t i;
  uint8_t bit;

  for (i = 0U; i < length; i++)
  {
    crc ^= data[i];
    for (bit = 0U; bit < 8U; bit++)
    {
      if ((crc & 0x0001U) != 0U)
      {
        crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
      }
      else
      {
        crc >>= 1U;
      }
    }
  }
  return crc;
}

ModbusRtu_Status_t ModbusRtu_ReadHoldingRegisters(UART_HandleTypeDef *huart,
                                                   GPIO_TypeDef *de_port,
                                                   uint16_t de_pin,
                                                   uint8_t slave_address,
                                                   uint16_t start_register,
                                                   uint16_t register_count,
                                                   uint16_t *registers,
                                                   uint32_t timeout_ms)
{
  uint8_t request[MODBUS_RTU_REQUEST_SIZE];
  uint8_t response[MODBUS_RTU_MAX_ADU_SIZE];
  uint16_t crc;
  uint16_t response_length;
  uint16_t i;

  if ((huart == NULL) || (de_port == NULL) || (registers == NULL) ||
      (register_count == 0U) || (register_count > 29U))
  {
    return MODBUS_RTU_ERR_ARGUMENT;
  }

  request[0] = slave_address;
  request[1] = 0x03U;
  request[2] = (uint8_t)(start_register >> 8U);
  request[3] = (uint8_t)(start_register & 0xFFU);
  request[4] = (uint8_t)(register_count >> 8U);
  request[5] = (uint8_t)(register_count & 0xFFU);
  crc = ModbusRtu_Crc16(request, 6U);
  request[6] = (uint8_t)(crc & 0xFFU);
  request[7] = (uint8_t)(crc >> 8U);

  (void)HAL_UART_AbortReceive(huart);
  (void)HAL_UART_AbortTransmit(huart);
  ModbusRtu_SetTransmit(de_port, de_pin);
  if (HAL_UART_Transmit(huart, request, sizeof(request), timeout_ms) != HAL_OK)
  {
    ModbusRtu_SetReceive(de_port, de_pin);
    return MODBUS_RTU_ERR_TRANSPORT;
  }
  while (__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) == RESET) { }
  ModbusRtu_SetReceive(de_port, de_pin);

  if (HAL_UART_Receive(huart, response, 3U, timeout_ms) != HAL_OK)
  {
    return MODBUS_RTU_ERR_TIMEOUT;
  }
  if ((response[0] != slave_address) || (response[1] != 0x03U) ||
      (response[2] != (uint8_t)(register_count * 2U)))
  {
    return MODBUS_RTU_ERR_RESPONSE;
  }

  /* Remaining bytes are payload plus CRC16. The first 3 bytes are already read. */
  response_length = (uint16_t)(response[2] + 2U);
  if ((response_length + 3U) > MODBUS_RTU_MAX_ADU_SIZE)
  {
    return MODBUS_RTU_ERR_RESPONSE;
  }
  if (HAL_UART_Receive(huart, &response[3], response_length, timeout_ms) != HAL_OK)
  {
    return MODBUS_RTU_ERR_TIMEOUT;
  }

  crc = ModbusRtu_Crc16(response, (uint16_t)(response[2] + 3U));
  if ((response[response[2] + 3U] != (uint8_t)(crc & 0xFFU)) ||
      (response[response[2] + 4U] != (uint8_t)(crc >> 8U)))
  {
    return MODBUS_RTU_ERR_CRC;
  }

  for (i = 0U; i < register_count; i++)
  {
    registers[i] = (uint16_t)(((uint16_t)response[3U + i * 2U] << 8U) |
                              response[4U + i * 2U]);
  }
  return MODBUS_RTU_OK;
}
