idf_component_register(SRCS "audio.c"
                       PRIV_REQUIRES ""
                       REQUIRES esp_driver_i2s esp_driver_gpio
                       INCLUDE_DIRS "include")
