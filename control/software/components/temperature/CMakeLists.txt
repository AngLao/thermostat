idf_component_register(SRCS "ds18b20.c" "temperature.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_timer driver)
