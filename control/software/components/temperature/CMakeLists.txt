idf_component_register(SRCS "driver_ds18b20_interface.c" "driver_ds18b20.c" "temperature.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_timer driver log)
