#ifndef _TEMPERATURE_H_  
#define _TEMPERATURE_H_


#include <stdio.h>

uint8_t temperature_device_online(void);
float get_expect_temperature(void);
float get_measure_temperature(void);
void set_expect_temperature(float temperature);
void temperature_task(void* arg);

#endif
