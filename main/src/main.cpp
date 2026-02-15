#include "main.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include <esp_random.h>
#include "nvs_flash.h"
#include <esp_netif.h>
#include <esp_event.h>
#include <ctime>
#include "esp_system.h"
#include "audio.h"
#include "networking.h"
#include "motionsensor.h"

EMBEDDED_FILE(boot_mp3);
EMBEDDED_FILE(howdy_mp3);
EMBEDDED_FILE(im_talkie_mp3);
EMBEDDED_FILE(offer_toast_1_mp3);
EMBEDDED_FILE(offer_toast_2_mp3);
EMBEDDED_FILE(waffle_man_mp3);
static const char *TAG = "Talkie";

static bool previously_detected = false;
static uint32_t detection_time = 0;
static uint32_t cooldown_time = 0;

extern "C"{
	void app_main();
}


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

void play_offer(){
	uint32_t num = esp_random();
    switch (num % 3)
    {
        case 0:
            QUEUE_AUDIO(offer_toast_1_mp3);
            break;
        case 1:
            QUEUE_AUDIO(offer_toast_2_mp3);
            break;
        case 2:
            QUEUE_AUDIO(waffle_man_mp3);
            break;
    }
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
	ESP_LOGI(TAG, "Flash Initialised");
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_LOGI(TAG, "TCP/IP Stack enabled.");
	ESP_ERROR_CHECK( esp_event_loop_create_default() );
	Wifi *wifi = Wifi::getInstance();
	ESP_ERROR_CHECK(esp_netif_init());

    audio_init();
    setup_motion_sensor();

    boot_sequence();
    vTaskDelay(20000 / portTICK_PERIOD_MS);
    wifi->init();
    // wifi->add_ap("SSID", "password");
    wifi->start();
    while(1)
    {
        time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);
		if(timeinfo.tm_year < 120)
        {
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			continue;
		}
        
		if(timeinfo.tm_year > 120 && (timeinfo.tm_hour >= 21 || timeinfo.tm_hour < 8))
        {   // It's night time so we prevent the sensor from detecting motion to avoid disturbing people.
            vTaskDelay(60000 / portTICK_PERIOD_MS);
            continue;
        }

        uint16_t distance = motion_sensor_get_distance();
        ESP_LOGI(TAG, "Distance: %u", distance);
        bool detected = motion_sensor_get_detected() && distance < 75;

        if (detected != previously_detected && !detection_time)
        {
            detection_time = esp_timer_get_time();
        }
        else if (detected == previously_detected && detection_time)
        {
            detection_time = 0;
        }



        if (detected && !previously_detected && detection_time &&(esp_timer_get_time() - detection_time > 1000000) && (esp_timer_get_time() >= cooldown_time))
        {
            previously_detected = true;
            ESP_LOGI(TAG, "Motion detected!");
            play_offer();
            detection_time = 0;
        }
        else if (!detected && previously_detected && detection_time && (esp_timer_get_time() - detection_time > 1000000))
        {
            cooldown_time = esp_timer_get_time() + 5*60*1000000;
            previously_detected = false;
            detection_time = 0;
        }
        vTaskDelay( pdMS_TO_TICKS(500) );
    }
    vTaskDelay(portMAX_DELAY);
    fflush(stdout);
    esp_restart();
}
