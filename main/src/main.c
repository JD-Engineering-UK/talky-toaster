#include "main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "audio.h"

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
    audio_init();

    boot_sequence();
    while(1){
        
        vTaskDelay( pdMS_TO_TICKS(30000) );
    }
    vTaskDelay(portMAX_DELAY);
    fflush(stdout);
    esp_restart();
}
