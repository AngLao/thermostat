#include "uart.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "interrupt.h"
#include "temperature.h"
#include "EasyPact.h" 

static uint8_t *data_buf = NULL;

static frame_t senbuf ; 	
static frame_t senbuf_target ; 

void uart0_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;
 
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
 
    data_buf = (uint8_t *) malloc(512);

}

void uart0_task(void *arg)
{ 
    while (1) {  
        int length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_0, (size_t*)&length));
        
        if(length != 0){
            length = uart_read_bytes(UART_NUM_0, data_buf, length, 0);
            frame_t* pframe = NULL;

            int count = 0;
            while (count < length)
            {
                if(easy_parse_data( &pframe ,data_buf[count]) == 0){
                    if(pframe->address == 'T' && pframe->id == '0'){ 
                        uint32_t data =0;
                        for (int var = 0; var < 4; var++) {
                            data += *((uint8_t*)pframe+4+var)<<(8*(3-var));
                        }
                        // 缩放100倍
                        float temperature_res = (int)data/100.0; 
                        set_expect_temperature(temperature_res);
                        
                    }
                
                    // ESP_LOGI("uart0:", "expect temperature change to: %0.2f", get_expect_temperature());
                }
                count++;
            }
        }
        
        easy_set_header(&senbuf, 0xAA);
        easy_set_address(&senbuf, 'W');
        easy_set_id(&senbuf, '0');
        easy_wipe_data(&senbuf);
        easy_add_data(&senbuf, (int) (get_measure_temperature()*100), 4); 
        easy_add_check(&senbuf);
        uart_write_bytes(UART_NUM_0, (uint8_t*)&senbuf, easy_return_buflen(&senbuf));

        easy_set_header(&senbuf, 0xAA);
        easy_set_address(&senbuf, 'W');
        easy_set_id(&senbuf, '1');
        easy_wipe_data(&senbuf);
        easy_add_data(&senbuf, (int) (get_expect_temperature()*100), 4); 
        easy_add_check(&senbuf);
        uart_write_bytes(UART_NUM_0, (uint8_t*)&senbuf, easy_return_buflen(&senbuf));

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}