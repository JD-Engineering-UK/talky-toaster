idf_component_register(SRCS "network.c"
                       PRIV_REQUIRES ""
                       REQUIRES esp_wifi nvs_flash
                       INCLUDE_DIRS "include")
