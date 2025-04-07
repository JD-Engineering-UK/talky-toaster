#include "main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "audio.h"
#include "network.h"

EMBEDDED_FILE(boot_mp3);
EMBEDDED_FILE(howdy_mp3);
EMBEDDED_FILE(im_talkie_mp3);
EMBEDDED_FILE(offer_toast_1_mp3);


void boot_sequence()
{
    QUEUE_AUDIO(boot_mp3);
    QUEUE_AUDIO(boot_mp3);
    QUEUE_AUDIO(boot_mp3);
    QUEUE_AUDIO(boot_mp3);
    
    QUEUE_AUDIO(howdy_mp3);
    QUEUE_AUDIO(im_talkie_mp3);
    QUEUE_AUDIO(offer_toast_1_mp3);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_init_sta();

    audio_init();


    


    boot_sequence();
    while(1){
        
        vTaskDelay( pdMS_TO_TICKS(30000) );
    }
    vTaskDelay(portMAX_DELAY);
    fflush(stdout);
    esp_restart();
}
