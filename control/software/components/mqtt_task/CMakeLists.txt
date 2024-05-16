idf_component_register(SRCS "mqtt_task.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES   mqtt
                                        wifi
                                        temperature
                                        interrupt
                                        pid
                        )
