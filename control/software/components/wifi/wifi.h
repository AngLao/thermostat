#ifndef _WIFI_
#define _WIFI_


#include <string.h>

uint8_t wifi_sta_isconnect(void);
void wifi_init(void);
void wifi_task(void *parameter);

#endif
