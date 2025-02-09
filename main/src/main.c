#include "main.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "audio.h"

extern const uint8_t drum_raw_start[] asm("_binary_drum_raw_start");
extern const uint8_t drum_raw_end[] asm("_binary_drum_raw_end");

void app_main(void)
{
    printf("Hello world!\n");
    audio_init();
    while(1){
        queue_audio(drum_raw_start - drum_raw_end, drum_raw_start);
        printf("Queued audio file\n");
        vTaskDelay( pdMS_TO_TICKS(30000) );
    }
    vTaskDelay(portMAX_DELAY);
    fflush(stdout);
    esp_restart();
}
