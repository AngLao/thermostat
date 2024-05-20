/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 * @file      driver_ds18b20_interface_template.c
 * @brief     driver ds18b20 interface template source file
 * @version   2.0.0
 * @author    Shifeng Li
 * @date      2021-04-06
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2021/04/06  <td>2.0      <td>Shifeng Li  <td>format the code
 * <tr><td>2020/12/20  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */
  
#include "driver_ds18b20_interface.h"

#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp32/rom/ets_sys.h"
#include "esp_timer.h"

uint8_t DS_GPIO;

/**
 * @brief  interface bus init
 * @return status code
 *         - 0 success
 *         - 1 bus init failed
 * @note   none
 */
uint8_t ds18b20_interface_init(void)
{
	DS_GPIO = GPIO_NUM_23;
	gpio_config_t ioConfig = {
		.pin_bit_mask = (1ull << GPIO_NUM_23),
		.mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
	};
	gpio_config(&ioConfig);
    return 0;
}

/**
 * @brief  interface bus deinit
 * @return status code
 *         - 0 success
 *         - 1 bus deinit failed
 * @note   none
 */
uint8_t ds18b20_interface_deinit(void)
{
    return 0;
}

/**
 * @brief      interface bus read
 * @param[out] *value points to a value buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       none
 */
uint8_t ds18b20_interface_read(uint8_t *value)
{
	(*value) = gpio_get_level(DS_GPIO);
    return 0;
}

/**
 * @brief     interface bus write
 * @param[in] value is the written value
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      none
 */
uint8_t ds18b20_interface_write(uint8_t value)
{
    gpio_set_level(DS_GPIO,value);
    return 0;
}

/**
 * @brief     interface delay ms
 * @param[in] ms
 * @note      none
 */
void ds18b20_interface_delay_ms(uint32_t ms)
{
	vTaskDelay(ms / portTICK_PERIOD_MS);
}

static __inline void delay_clock(int ts)
{
    uint32_t start, curr;

    __asm__ __volatile__("rsr %0, ccount" : "=r"(start));
    do
    __asm__ __volatile__("rsr %0, ccount" : "=r"(curr));
    while (curr - start <= ts);
}
/**
 * @brief     interface delay us
 * @param[in] us
 * @note      none
 */
void ds18b20_interface_delay_us(uint32_t us)
{
    // ets_delay_us(us);
    delay_clock(240*us);
}

/**
 * @brief interface enable the interrupt
 * @note  none
 */
void ds18b20_interface_enable_irq(void)
{
    taskENABLE_INTERRUPTS();
}

/**
 * @brief interface disable the interrupt
 * @note  none
 */
void ds18b20_interface_disable_irq(void)
{
    taskDISABLE_INTERRUPTS();
}

/**
 * @brief     interface print format data
 * @param[in] fmt is the format data
 * @note      none
 */
void ds18b20_interface_debug_print(const char *const fmt, ...)
{
    // printf(fmt);
}
