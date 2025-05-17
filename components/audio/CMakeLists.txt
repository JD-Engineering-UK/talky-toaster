idf_component_register(
        SRCS "audio.c"
        INCLUDE_DIRS "include"
        REQUIRES "board audio_sal" "audio_stream" "esp_dispatcher" "esp_peripherals"
)