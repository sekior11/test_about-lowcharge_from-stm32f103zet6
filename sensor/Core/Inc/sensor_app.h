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

#endif /* __SENSOR_APP_H */
