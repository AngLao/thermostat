idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES    uart 
                                interrupt 
                                temperature 
                                pid
                                wifi
                                esp_wifi
                                nvs_flash
                                mqtt_task
                                OLED
                        )
