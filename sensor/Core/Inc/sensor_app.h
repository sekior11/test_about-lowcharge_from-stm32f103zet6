#ifndef __SENSOR_APP_H
#define __SENSOR_APP_H

#include "iwt603.h"

typedef struct
{
  IWT603_Data_t attitude;
} SensorApp_Data_t;

void SensorApp_Init(void);
void SensorApp_Process(void);
const SensorApp_Data_t *SensorApp_GetData(void);

/* STOP 低功耗新增: 虚拟时间基准 + 唤醒控制 */
uint32_t SensorApp_GetVirtualMs(void);
uint8_t SensorApp_IsAlarmSessionActive(void);
void SensorApp_RestartIwt603Dma(void);

#endif /* __SENSOR_APP_H */
