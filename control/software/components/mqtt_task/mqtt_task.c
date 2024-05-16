#include "mqtt_task.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "wifi.h"
#include "temperature.h"
#include "interrupt.h"
#include "pid.h"

static const char *TAG = "mqtt";

//客户端配置
#define CONFIG_BROKER_URI "ws://broker.emqx.io:8083/mqtt"
#define MQTT_CLIENT_USERNAME ""
#define MQTT_CLIENT_PASSWORD ""
#define MQTT_CLIENT_ID ""

//初始化要订阅的主题
#define EXPECT_TEMPERATURE_TOPIC "set_temp" 
#define FAN1_TOPIC  "fan1" 
#define FAN2_TOPIC  "fan2" 
#define KP_TOPIC    "kp" 
#define KI_TOPIC    "ki" 

//初始化需要订阅的主题
struct mqtt_subscrib_t mqtt_subscrib_buf[] = {  {.topic = EXPECT_TEMPERATURE_TOPIC},
                                                {.topic = FAN1_TOPIC},
                                                {.topic = FAN2_TOPIC},
                                                {.topic = KP_TOPIC},
                                                {.topic = KI_TOPIC},
                                            };
static char mqtt_connect_status = 0;

static void mqtt_connect_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    ESP_LOGI(TAG, "mqtt connect successful");
    mqtt_connect_status = 1;

    int subscrib_count = sizeof(mqtt_subscrib_buf)/sizeof(struct mqtt_subscrib_t);
    printf("need to subscribe %d topics\n", subscrib_count);

    //发送订阅请求
    for (int i = 0; i < subscrib_count; i++)
    {
        mqtt_subscrib_buf[i].msgid = esp_mqtt_client_subscribe(client, mqtt_subscrib_buf[i].topic, 0);
        printf("subscribing topic: %s\n", mqtt_subscrib_buf[i].topic);
    }
    
}

static void mqtt_disconnect_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    mqtt_connect_status = 0;
}

static void mqtt_subscrib_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    //根据返回id判断订阅是否成功
    int subscrib_count = sizeof(mqtt_subscrib_buf)/sizeof(struct mqtt_subscrib_t);
    for (int i = 0; i < subscrib_count; i++)
    {
        if(event->msg_id == mqtt_subscrib_buf[i].msgid){
            printf("success subscribe topic: %s\n", mqtt_subscrib_buf[i].topic);
            return;
        }
    }
}

static void mqtt_data_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    float res = atof(event->data);
    printf("event topic: %s\t", event->topic);
    printf("event data: %0.2f\n", res);

    if(strncmp(event->topic,EXPECT_TEMPERATURE_TOPIC, event->topic_len) == 0){
        set_expect_temperature(res);
        printf("set_expect_temperature: %0.2f\n", res);
    }else if(strncmp(event->topic,FAN1_TOPIC, event->topic_len) == 0){
        if((int)res == 1){
            set_fan_output(1, 1);
            printf("fan1 is open\n");
        }
        else if((int)res == 0){
            set_fan_output(1, 0);
            printf("fan1 is close\n");
        }
    }else if(strncmp(event->topic,FAN2_TOPIC, event->topic_len) == 0){
        if((int)res == 1){
            set_fan_output(2, 1);
            printf("fan2 is open\n");
        }
        else if((int)res == 0){
            set_fan_output(2, 0);
            printf("fan2 is close\n");
        }
    }else if(strncmp(event->topic,KP_TOPIC, event->topic_len) == 0){
        set_temperature_controller_kp(res);
    }else if(strncmp(event->topic,KI_TOPIC, event->topic_len) == 0){
        set_temperature_controller_ki(res);
    }



    memset(event->topic, 0, (event->topic_len+event->data_len)); 
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_error_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
        ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

    }
}

char mqtt_is_connect(void)
{
    return mqtt_connect_status;
}

void mqtt_task(void)
{
    while(!wifi_sta_isconnect())
        vTaskDelay(2000/portTICK_PERIOD_MS);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URI,
        .credentials.username = MQTT_CLIENT_USERNAME,
        .credentials.authentication.password = MQTT_CLIENT_PASSWORD,
        .credentials.client_id = MQTT_CLIENT_ID,
        .session.keepalive = 20
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ERROR, mqtt_error_handler, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DATA, mqtt_data_handler, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED, mqtt_connect_handler, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_DISCONNECTED, mqtt_disconnect_handler, NULL);
    esp_mqtt_client_register_event(client, MQTT_EVENT_SUBSCRIBED, mqtt_subscrib_handler, NULL);
    
    esp_mqtt_client_start(client);

    while (true)
    {
        if(mqtt_is_connect()){        
            char str[25] = {0};
            int str_len = 0;
            str_len = sprintf (str, "%.2f",get_expect_temperature());
            esp_mqtt_client_publish(client, "trage", str, str_len, 0, 0);
            str_len = sprintf (str, "%.2f",get_measure_temperature());
            esp_mqtt_client_publish(client, "measure", str, str_len, 0, 0);
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
    

}
