#include <stdio.h>
#include "esp_log.h"
#include "JD_FOTA.h"
#include "http_lib.h"
#include "cJSON.h"

#define MAX_HTTP_OUTPUT_BUFFER 1024

static const char *TAG = "FOTA";
#define FIRMWARE_API_BASE "https://jdengineering.uk"
#define FIRMWARE_QUERY_ENDPOINT "/iot_hub/firmware_images/?device_model=Talky%20Toaster&json"


static char response_buffer[MAX_HTTP_OUTPUT_BUFFER];

bool check_latest(void)
{
    bool have_latest = true; // Default to true so if the server is down it assumes latest.

    uint32_t content_length = http_get(FIRMWARE_API_BASE FIRMWARE_QUERY_ENDPOINT, response_buffer, MAX_HTTP_OUTPUT_BUFFER);
    if (content_length > 0) 
    {
        ESP_LOGI(TAG, "Received firmware version response: %.*s", content_length, response_buffer);
        cJSON *json = cJSON_ParseWithLength(response_buffer, content_length);
        if (json) 
        {
            cJSON *version_item = cJSON_GetObjectItem(json, "latest_version");
            if (version_item && cJSON_IsString(version_item)) 
            {
                const char *latest_version = version_item->valuestring;
                ESP_LOGI(TAG, "Latest firmware version from server: %s", latest_version);
            }
            else 
            {
                ESP_LOGE(TAG, "Invalid JSON response: 'latest_version' not found or not a string");
            }
            cJSON_Delete(json);
        } 
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to get firmware version from server");
    }

    return have_latest;
}


