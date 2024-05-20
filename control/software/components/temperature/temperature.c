#include "temperature.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "math.h"

#include "driver_ds18b20.h"
#include "driver_ds18b20_interface.h"

//预留了4个温度传感器接口
static uint8_t rom_buf[4][8];
static ds18b20_handle_t ds18b20_sensor;    
static ds18b20_resolution_t resolution = DS18B20_RESOLUTION_11BIT;
static uint8_t online_device = 0;

static const float MAX_EXPECT_TEMPERATURE = 70.00;
static const float MIN_EXPECT_TEMPERATURE = 20.00;
static float expect_temperature = 45.00;
static float measure_temperature = 0.00;

float raw_temperature = 0;

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
	return online_device;
}

void set_expect_temperature(float temperature)
{
	expect_temperature = temperature;
	if(expect_temperature > MAX_EXPECT_TEMPERATURE)
		expect_temperature = MAX_EXPECT_TEMPERATURE;
	else if(expect_temperature < MIN_EXPECT_TEMPERATURE)
		expect_temperature = MIN_EXPECT_TEMPERATURE;
}


// 卡尔曼滤波
static float kalman_filter(float z)
{
    static float x_hat = 0; // 估计的状态值
    static float P = 0;     // 估计的误差协方差
    static float Q = 0.001;  // 测量噪声协方差
    static float R = 0.02;   // 过程噪声协方差

    // 预测
    float x_hat_minus = x_hat;
    float P_minus = P + Q;

    // 更新
    float K = P_minus / (P_minus + R);
    x_hat = x_hat_minus + K * (z - x_hat_minus);
    P = (1 - K) * P_minus;

    return x_hat;
}

void temperature_task(void* arg)
{
    /* link interface function */
    DRIVER_DS18B20_LINK_INIT(&ds18b20_sensor, ds18b20_handle_t);
    DRIVER_DS18B20_LINK_BUS_INIT(&ds18b20_sensor, ds18b20_interface_init);
    DRIVER_DS18B20_LINK_BUS_DEINIT(&ds18b20_sensor, ds18b20_interface_deinit);
    DRIVER_DS18B20_LINK_BUS_READ(&ds18b20_sensor, ds18b20_interface_read);
    DRIVER_DS18B20_LINK_BUS_WRITE(&ds18b20_sensor, ds18b20_interface_write);
    DRIVER_DS18B20_LINK_DELAY_MS(&ds18b20_sensor, ds18b20_interface_delay_ms);
    DRIVER_DS18B20_LINK_DELAY_US(&ds18b20_sensor, ds18b20_interface_delay_us);
    DRIVER_DS18B20_LINK_ENABLE_IRQ(&ds18b20_sensor, ds18b20_interface_enable_irq);
    DRIVER_DS18B20_LINK_DISABLE_IRQ(&ds18b20_sensor, ds18b20_interface_disable_irq);
    DRIVER_DS18B20_LINK_DEBUG_PRINT(&ds18b20_sensor, ds18b20_interface_debug_print);
    
    /* ds18b20 init */
ds18b20_init :    
    while (ds18b20_init(&ds18b20_sensor) != 0)
    {
        vTaskDelay(300/portTICK_PERIOD_MS);
    }
     
    
    /* set skip rom mode */
    if (ds18b20_set_mode(&ds18b20_sensor, DS18B20_MODE_SKIP_ROM) != 0)
    {
        ds18b20_interface_debug_print("ds18b20: set mode failed.\n");
        (void)ds18b20_deinit(&ds18b20_sensor);
        goto ds18b20_init;
    }
    
    /* set default resolution */
    if (ds18b20_scratchpad_set_resolution(&ds18b20_sensor, resolution) != 0)
    {
        ds18b20_interface_debug_print("ds18b20: scratchpad set resolution failed.\n");
        (void)ds18b20_deinit(&ds18b20_sensor);
        goto ds18b20_init;
    }
    
    ds18b20_interface_debug_print("ds18b20_sensor init success ---------------------------.\n");


	while (1) {
        // uint8_t num = 2;
        // if(ds18b20_search_rom(&ds18b20_sensor, rom_buf, &num) == 0){
        //     ds18b20_get_rom(&ds18b20_sensor, rom_buf);
        //     for (size_t x = 0; x < online_device; x++)
        //     {
        //         printf("ds18b20_sensor %d rom : 0X ", x);
        //         for (size_t y = 0; y < 8; y++)
        //         {
        //             printf("%X ", rom_buf[x][y]);
        //         }
        //         printf("\n");
        //     }
        // }

    	int16_t raw;
        float raw_temperature = 0;
        if(ds18b20_read(&ds18b20_sensor, (int16_t *)&raw, &raw_temperature) != 0){
            online_device = 0;
            goto ds18b20_init;
        }
        online_device = 1;
        //数据滤波
		measure_temperature = kalman_filter(raw_temperature);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
