idf_component_register(SRCS "EasyPact.c" "uart.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES driver
                                  temperature
                                  interrupt
                    )
