idf_component_register(
        SRCS "time.cpp" "credentials.cpp" "wifi.cpp"
        INCLUDE_DIRS "include"
        REQUIRES "esp_wifi" "nvs_flash"
)
