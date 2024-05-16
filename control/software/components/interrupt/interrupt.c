#include "interrupt.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define KEY1 GPIO_NUM_21
#define KEY2 GPIO_NUM_22
#define ZERO GPIO_NUM_19
#define GATE GPIO_NUM_32
#define FAN1 GPIO_NUM_33
#define FAN2 GPIO_NUM_25

//消息队列句柄
static QueueHandle_t key1_queue = NULL;
static QueueHandle_t key2_queue = NULL;
static gptimer_handle_t gptimer = NULL;

//伪占空比，过零周期100hz，即控制每10ms内晶闸管何时打开以控制能量输出占比
static int key_step = 1000; //(us)
static int time_count = 6666; //(us)
static int MIN_COUNT = 0; //(us)
static int MAX_COUNT = 8000; //(us)

static char fan1_status = 0;
static char fan2_status = 0;

static void IRAM_ATTR key1_isr(void* arg)
{
    if(gpio_get_level(KEY1) == 0){
        *(bool*)arg = true; 
        xQueueSendFromISR(key1_queue,(bool*)arg,NULL);
    }
}

static void IRAM_ATTR key2_isr(void* arg)
{
    if(gpio_get_level(KEY2) == 0){
        *(bool*)arg = true; 
        xQueueSendFromISR(key2_queue,(bool*)arg,NULL);
    }
}

static void IRAM_ATTR zero_isr(void* arg)
{
    if(gpio_get_level(ZERO) == 0){
        ESP_ERROR_CHECK(gptimer_set_raw_count(gptimer, time_count));
    }
}

static bool IRAM_ATTR timer_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    
    gpio_set_level(GATE, 1);
    for(int i= 100; i> 0; i--)
        __asm__("nop");
    gpio_set_level(GATE, 0);
    // return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}

static void gpio_init(void)
{
    // //key1
    // gpio_set_direction(KEY1,GPIO_MODE_INPUT);
    // gpio_set_pull_mode(KEY1,GPIO_FLOATING);
    // gpio_set_intr_type(KEY1,GPIO_INTR_NEGEDGE);

    // static bool key1_flag = false;
    // gpio_isr_handler_add(KEY1,key1_isr,&key1_flag);
    
    // key1_queue = xQueueCreate( 1,sizeof(bool));
    // if(key1_queue == NULL){
    //     ESP_LOGI("key1_queue", "消息队列创建失败，函数返回");
    //     return ;
    // }

    // //key2
    // gpio_set_direction(KEY2,GPIO_MODE_INPUT);
    // gpio_set_pull_mode(KEY2,GPIO_FLOATING);
    // gpio_set_intr_type(KEY2,GPIO_INTR_NEGEDGE);

    // static bool key2_flag = false;
    // gpio_isr_handler_add(KEY2,key2_isr,&key2_flag);

    // key2_queue = xQueueCreate( 1,sizeof(bool));
    // if(key2_queue == NULL){
    //     ESP_LOGI("key2_queue", "消息队列创建失败，函数返回");
    //     return ;
    // }

    //过零检测中断
    gpio_set_direction(ZERO,GPIO_MODE_INPUT);
    gpio_set_pull_mode(ZERO,GPIO_FLOATING);
    gpio_set_intr_type(ZERO,GPIO_INTR_NEGEDGE);

    static bool zero_flag = false;
    gpio_isr_handler_add(ZERO,zero_isr,&zero_flag);

    //可控硅输出引脚
    gpio_set_direction(GATE,GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(GATE,GPIO_PULLDOWN_ONLY);

    //风扇输出引脚
    gpio_set_direction(FAN1,GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(FAN1,GPIO_PULLDOWN_ONLY);
    gpio_set_direction(FAN2,GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(FAN2,GPIO_PULLDOWN_ONLY);

}

void timer_init(void)
{
    // 初始化定时器
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_DOWN,  
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

	// 注册定时器事件回调函数(警报事件的回调函数)
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_isr,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    // 使能定时器
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 10000,
        .flags.auto_reload_on_alarm = true,
        .alarm_count = 0, // 触发警报事件的目标计数值 = 10ms = 10000 x 1us
    };
	// 设置定时器的报警事件
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
	// 启动定时器
    ESP_ERROR_CHECK(gptimer_start(gptimer));  
            
}

void set_thyristor_output(int time)
{
    time_count = time;
    if(time_count < MIN_COUNT)
        time_count = MIN_COUNT;

    if(time_count > MAX_COUNT)
        time_count = MAX_COUNT;
}

void set_fan_output(int fan_inex, int mode)
{
    if(fan_inex == 1){
        gpio_set_level(FAN1, mode);
        fan1_status = mode;
    }
    else if(fan_inex == 2){
        gpio_set_level(FAN2, mode);
        fan2_status = mode;
    }
}

char get_fan_mode(int fan_inex)
{
    if(fan_inex == 1){
        return fan1_status;
    }
    else if(fan_inex == 2){
        return fan2_status;
    }
    return -1;
}

int get_pwm_value(void)
{
    return time_count;
}

void interrupt_task(void* arg)
{
    bool flag = false;
    bool temp = false; 

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));

    gpio_init();
    timer_init();

    while(true){
        // //按下key1
        // xQueueReceive(key1_queue,&flag,0);
        // if(flag == true){
        //     flag = false; 
        //     xQueueSend(key1_queue,&temp,0);
        // }
        
        // //按下key2
        // xQueueReceive(key2_queue,&flag,0);
        // if(flag == true){
        //     flag = false; 
        //     xQueueSend(key2_queue,&temp,0);
        // }

        vTaskDelay(40/portTICK_PERIOD_MS);
    }
}