set(COMPONENT_SRCS "audio.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")
set(COMPONENT_REQUIRES board audio_sal audio_stream esp_dispatcher esp_peripherals)


register_component()
