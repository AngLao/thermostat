#ifndef _MQTT_TASK_H_
#define _MQTT_TASK_H_

struct mqtt_subscrib_t
{
    int msgid;
    char *topic;
};

void mqtt_task(void);
char mqtt_is_connect(void);
char get_fan_mode(int fan_inex);

#endif
