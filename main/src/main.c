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
    extern const uint8_t boot_raw_start[] asm("_binary_boot_raw_start");
    extern const uint8_t boot_raw_end[] asm("_binary_boot_raw_end");
    const size_t boot_raw_size = (boot_raw_end - boot_raw_start);
    
    extern const uint8_t howdy_raw_start[] asm("_binary_howdy_raw_start");
    extern const uint8_t howdy_raw_end[] asm("_binary_howdy_raw_end");
    const size_t howdy_raw_size = (howdy_raw_end - howdy_raw_start);

    // extern const uint8_t im_talkie_raw_start[] asm("_binary_im_talkie_raw_start");
    // extern const uint8_t im_talkie_raw_end[] asm("_binary_im_talkie_raw_end");
    // const size_t im_talkie_raw_size = (im_talkie_raw_end - im_talkie_raw_start);

    queue_audio(boot_raw_size, boot_raw_start);
    queue_audio(boot_raw_size, boot_raw_start);
    queue_audio(boot_raw_size, boot_raw_start);
    queue_audio(boot_raw_size, boot_raw_start);
    queue_audio(boot_raw_size, boot_raw_start);
    queue_audio(howdy_raw_size, howdy_raw_start);
    // queue_audio(im_talkie_raw_size, im_talkie_raw_start);
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
