#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_netif.h"
#include "esp_event.h"

#include "uart.h"
#include "interrupt.h"
#include "temperature.h"
#include "PID.h"
#include "wifi.h"
#include "mqtt_task.h"

#include "OLED.h"

void oled_task(void* arg)
{
    // IIC总线主机初始化
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI("oled", "I2C initialized successfully");

    // OLED屏幕初始化
    OLED_Init();
    OLED_ShowString(0,  0, "wifi:", 8);
    OLED_ShowString(0,  1, "mqtt:", 8);
    OLED_ShowString(0,  2, "setT:",  8);
    OLED_ShowString(0,  3, "temp:", 8);
    OLED_ShowString(0,  4, "fan1:", 8);
    OLED_ShowString(0,  5, "fan2:", 8);
    OLED_ShowString(0,  6, "Kp:",   8);
    OLED_ShowString(64, 6, "Ki:",   8);
    OLED_ShowString(0,  7, "PWM:",   8);

    while (true)
    {    
        //连接状态显示更新
        if(wifi_sta_isconnect())
            OLED_ShowString(45, 0, "   connect", 8);
        else
            OLED_ShowString(45, 0, "disconnect", 8);

        if(mqtt_is_connect())
            OLED_ShowString(45, 1, "   connect", 8);
        else
            OLED_ShowString(45, 1, "disconnect", 8);
        
        //温度状态显示更新
        OLED_Showdecimal(80, 2, get_expect_temperature(), 2, 2, 8);
        OLED_ShowString(115, 2, "C", 8);
        OLED_Showdecimal(80, 3, get_measure_temperature(), 2, 2, 8);
        OLED_ShowString(115, 3, "C", 8);
        
        //风扇运行状态显示更新
        if(get_fan_mode(1))
            OLED_ShowString(45, 4, "      open", 8);
        else
            OLED_ShowString(45, 4, "     close", 8);
        if(get_fan_mode(2))
            OLED_ShowString(45, 5, "      open", 8);
        else
            OLED_ShowString(45, 5, "     close", 8);
            
        //PID参数显示更新
        OLED_ShowNum(25, 6, -get_temperature_controller_kp(), 5, 8);
        OLED_ShowNum(90, 6, -get_temperature_controller_ki(), 5, 8);
        
        //PWM输出显示更新
        OLED_ShowNum(90, 7, get_pwm_value(), 5, 8);

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    /* 系统基础配置初始化 */
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    uart0_init();
    wifi_init();
    
    xTaskCreate(uart0_task, "uart0_task", 1024*2, NULL, 3, NULL);

    xTaskCreate(interrupt_task, "interrupt_task", 1024*2, NULL, 3, NULL);

    xTaskCreate(temperature_task, "temperature_task", 1024*2, NULL, 3, NULL);

    xTaskCreate(control_task, "control_task", 1024*2, NULL, 3, NULL);

    xTaskCreate(wifi_task, "wifi_task", 1024*4, NULL, 3, NULL);

    xTaskCreate(mqtt_task, "mqtt_task", 1024*2, NULL, 3, NULL);

    xTaskCreate(oled_task, "oled_task", 1024*2, NULL, 2, NULL);
}
