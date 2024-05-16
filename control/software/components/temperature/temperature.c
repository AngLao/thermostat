#include "temperature.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define TEMP_BUS GPIO_NUM_23
#define digitalWrite gpio_set_level
static int resolution = 11;
//预留了4个温度传感器接口
static uint8_t online_device = 0;
static DeviceAddress tempSensors[4];

static const float MAX_EXPECT_TEMPERATURE = 70.00;
static const float MIN_EXPECT_TEMPERATURE = 20.00;
static float expect_temperature = 45.00;
static float measure_temperature = 0.00;

float raw_temperature = 0;

void getTempAddresses(DeviceAddress *tempSensorAddresses) 
{
	unsigned int numberFound = 0;
	reset_search();
	// search for 2 addresses on the oneWire protocol
	while (search(tempSensorAddresses[numberFound],true)) {
		vTaskDelay(50 / portTICK_PERIOD_MS);
		numberFound++;
		if (numberFound == 2) break;
	}
	return;
}

float get_expect_temperature(void) 
{
	return expect_temperature;
}

float get_measure_temperature(void) 
{
	return measure_temperature;
}

uint8_t temperature_device_online(void)
{
	if(online_device == 0)
		return 0;
	else
		return 1;
}

void set_expect_temperature(float temperature)
{
	expect_temperature = temperature;
	if(expect_temperature > MAX_EXPECT_TEMPERATURE)
		expect_temperature = MAX_EXPECT_TEMPERATURE;
	else if(expect_temperature < MIN_EXPECT_TEMPERATURE)
		expect_temperature = MIN_EXPECT_TEMPERATURE;
}

static void search_single_device(void)
{
	online_device = 0;
	while (true) {
		if (!search(tempSensors[0],true)) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
			continue;
		}
		ds18b20_reset();
		ds18b20_setResolution(tempSensors,1,resolution);
		break;
	}
	online_device = 1;
}

static float mid_filter(float value)
{
	static const char N = 12;
	unsigned int count, i, j, temp;
    float value_buf[N];
    float  sum = 0.0;
    for( count = 0; count < N; count++ )
    {
        value_buf[count] =  value;
    }
    for( j = 0; j < N - 1; j++ )
    {
        for( i = 0; i < N - j - 1; i++ )
        {
            if( value_buf[i] > value_buf[i + 1] )
            {
                temp = value_buf[i];
                value_buf[i] = value_buf[i + 1];
                value_buf[i + 1] = temp;
            }
        }
    }
    for( count = 2; count < N - 2; count++ )
    {
        sum += value_buf[count];
    }
    return (float)( sum / ( N - 4 ) );
}

void temperature_task(void* arg)
{
	ds18b20_init(TEMP_BUS);
	search_single_device();
	while (1) {
		ds18b20_requestTemperatures();
		raw_temperature = ds18b20_getTempC(tempSensors[0]);

		//数据异常
		if((int)raw_temperature < 10 || (int)raw_temperature > 80){
			search_single_device();
			continue;
		}
		measure_temperature = mid_filter(raw_temperature);//获取滤波数据
		
		// printf("measure_temperature: %0.2f°C expect_temperature: %0.2f°C\n", measure_temperature, expect_temperature);
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}
