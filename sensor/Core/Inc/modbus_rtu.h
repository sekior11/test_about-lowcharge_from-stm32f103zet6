#ifndef __MODBUS_RTU_H
#define __MODBUS_RTU_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define MODBUS_RTU_MAX_ADU_SIZE 64U

typedef enum
{
  MODBUS_RTU_OK = 0,
  MODBUS_RTU_ERR_ARGUMENT,
  MODBUS_RTU_ERR_TRANSPORT,
  MODBUS_RTU_ERR_TIMEOUT,
  MODBUS_RTU_ERR_CRC,
  MODBUS_RTU_ERR_RESPONSE
} ModbusRtu_Status_t;

uint16_t ModbusRtu_Crc16(const uint8_t *data, uint16_t length);
ModbusRtu_Status_t ModbusRtu_ReadHoldingRegisters(UART_HandleTypeDef *huart,
                                                   GPIO_TypeDef *de_port,
                                                   uint16_t de_pin,
                                                   uint8_t slave_address,
                                                   uint16_t start_register,
                                                   uint16_t register_count,
                                                   uint16_t *registers,
                                                   uint32_t timeout_ms);

#endif /* __MODBUS_RTU_H */
