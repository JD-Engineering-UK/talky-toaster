idf_component_register(SRCS "src/main.c"
                       PRIV_REQUIRES spi_flash motion_sensor audio
                       EMBED_FILES drum.raw
                       REQUIRES "" 
                       INCLUDE_DIRS "include")
