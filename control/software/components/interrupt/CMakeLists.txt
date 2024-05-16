idf_component_register(SRCS "interrupt.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES driver log esp_timer)
