#include "main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "audio.h"

void boot_sequence()
{
    extern const uint8_t boot_mp3_start[] asm("_binary_boot_mp3_start");
    extern const uint8_t boot_mp3_end[] asm("_binary_boot_mp3_end");
    const size_t boot_mp3_size = (boot_mp3_end - boot_mp3_start);
    
    extern const uint8_t howdy_mp3_start[] asm("_binary_howdy_mp3_start");
    extern const uint8_t howdy_mp3_end[] asm("_binary_howdy_mp3_end");
    const size_t howdy_mp3_size = (howdy_mp3_end - howdy_mp3_start);

    extern const uint8_t im_talkie_mp3_start[] asm("_binary_im_talkie_mp3_start");
    extern const uint8_t im_talkie_mp3_end[] asm("_binary_im_talkie_mp3_end");
    const size_t im_talkie_mp3_size = (im_talkie_mp3_end - im_talkie_mp3_start);

    queue_audio(boot_mp3_size, boot_mp3_start);
    queue_audio(boot_mp3_size, boot_mp3_start);
    queue_audio(boot_mp3_size, boot_mp3_start);
    queue_audio(boot_mp3_size, boot_mp3_start);
    queue_audio(boot_mp3_size, boot_mp3_start);
    queue_audio(howdy_mp3_size, howdy_mp3_start);
    queue_audio(im_talkie_mp3_size, im_talkie_mp3_start);
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
