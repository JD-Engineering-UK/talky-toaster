idf_component_register(SRCS "src/main.c"
                       PRIV_REQUIRES spi_flash motion_sensor audio board
                       EMBED_FILES "audio_files/howdy.raw" "audio_files/boot.raw"
                       REQUIRES "" 
                       INCLUDE_DIRS "include")
