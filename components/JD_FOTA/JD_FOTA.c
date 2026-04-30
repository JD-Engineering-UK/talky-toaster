#include <stdio.h>
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_image_format.h"
#include "esp_system.h"
#include "JD_FOTA.h"
#include "http_lib.h"
#include "cJSON.h"

#define MAX_HTTP_OUTPUT_BUFFER 1024

static const char *TAG = "FOTA";
#define FIRMWARE_API_BASE "https://jdengineering.uk"
#define FIRMWARE_QUERY_ENDPOINT "/iot_hub/api/firmware_images/?device_model=Talky%20Toaster"


static char response_buffer[MAX_HTTP_OUTPUT_BUFFER];
static char firmware_url[256]; // Buffer to hold the firmware URL for downloading

static bool image_header_was_checked = false;
static int binary_file_length = 0;
static const esp_partition_t *update_partition = NULL;
static esp_ota_handle_t update_handle = 0;
static esp_partition_t *running = NULL;
static esp_partition_t *configured = NULL;
static esp_app_desc_t running_app_info;
static esp_err_t err;
static uint32_t buffer_level = 0;

bool check_latest()
{
    bool have_latest = true; // Default to true so if the server is down it assumes latest.
    configured = esp_ota_get_boot_partition();
    running = esp_ota_get_running_partition();
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }
    uint32_t content_length = http_get(FIRMWARE_API_BASE FIRMWARE_QUERY_ENDPOINT, response_buffer, MAX_HTTP_OUTPUT_BUFFER);
    if (content_length > 0) 
    {
        // ESP_LOGI(TAG, "Received firmware version response: %.*s", content_length, response_buffer);
        cJSON *json = cJSON_ParseWithLength(response_buffer, content_length);
        if (json) 
        {
            cJSON *version_item = cJSON_GetObjectItem(json, "latest");
            if (version_item && cJSON_IsObject(version_item)) 
            {
                const char *latest_version = cJSON_GetObjectItem(version_item, "version")->valuestring;
                if (strcmp(latest_version, running_app_info.version) == 0) 
                {
                    have_latest = true;
                    // ESP_LOGI(TAG, "Device is running the latest firmware version: %s", running_app_info.version);
                } 
                else 
                {
                    have_latest = false;
                    sprintf(firmware_url, "%s%s", FIRMWARE_API_BASE, cJSON_GetObjectItem(version_item, "file")->valuestring);
                    // ESP_LOGI(TAG, "Device firmware version (%s) is outdated. Latest version available: %s", running_app_info.version, latest_version);
                }
                // ESP_LOGI(TAG, "Latest firmware version from server: %s", latest_version);
            }
            else 
            {
                // ESP_LOGE(TAG, "Invalid JSON response: 'latest' not found or not an object");
            }
            cJSON_Delete(json);
        } 
    } 
    else 
    {
        // ESP_LOGE(TAG, "Failed to get firmware version from server");
    }

    return have_latest;
}

void fota_download_callback(const char *data, int len)
{
    if(err != ESP_OK) 
    {
        // ESP_LOGE(TAG, "Error in OTA update process, aborting");
        return;
    }

    memcpy(response_buffer + buffer_level, data, len);
    buffer_level += len;

    if (image_header_was_checked == false) 
    {
        esp_app_desc_t new_app_info;
        if (buffer_level > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) 
        {
            // check current version with downloading
            memcpy(&new_app_info, &response_buffer[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
            ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

            esp_app_desc_t running_app_info;
            if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) 
            {
                ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
            }

            const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
            esp_app_desc_t invalid_app_info;
            if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) 
            {
                ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
            }

            // check current version with last invalid partition
            if (last_invalid_app != NULL) 
            {
                if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) 
                {
                    ESP_LOGW(TAG, "New version is the same as invalid version.");
                    ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                    ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                }
            }
#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
            if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) 
            {
                ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
            }
#endif

            image_header_was_checked = true;

            err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
            if (err != ESP_OK) 
            {
                ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                esp_ota_abort(update_handle);
            }
            ESP_LOGI(TAG, "esp_ota_begin succeeded");
        } 
        else 
        {
            return;
        }
    }
    err = esp_ota_write( update_handle, (const void *)response_buffer, buffer_level);
    if (err != ESP_OK) 
    {
        esp_ota_abort(update_handle);
        ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
        return;
    }
    binary_file_length += buffer_level;
    buffer_level = 0;
    ESP_LOGD(TAG, "Written image length %d", binary_file_length);
}


void start_latest_firmware_update()
{
    // First check if we have the latest firmware
    if (check_latest()) 
    {
        ESP_LOGI(TAG, "Already running the latest firmware version");
        return;
    }

    // Latest check saves the latest URL so no need to query again

    start_firmware_update(firmware_url);
}

void start_firmware_update(const char *url)
{
    image_header_was_checked = false;
    update_partition = esp_ota_get_next_update_partition(NULL);
    binary_file_length = 0;
    buffer_level = 0;
    err = ESP_OK;
    esp_ota_abort(update_handle);
    // Start downloading the firmware binary
    http_download(url, fota_download_callback);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Firmware update failed: %s", esp_err_to_name(err));
        return;
    }
    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        return;
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        return; 
    }
    ESP_LOGI(TAG, "Firmware update successful, rebooting...");
    esp_restart();
}
