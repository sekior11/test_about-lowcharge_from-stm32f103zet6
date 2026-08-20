#ifndef __DATA_RECEIVER_H
#define __DATA_RECEIVER_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define DATA_RECEIVER_BUFFER_SIZE 256U

void DataReceiver_Init(UART_HandleTypeDef *huart);
void DataReceiver_RxByte(uint8_t byte);
uint16_t DataReceiver_Available(void);
uint8_t DataReceiver_Read(uint8_t *byte);
uint32_t DataReceiver_OverflowCount(void);

#endif /* __DATA_RECEIVER_H */
