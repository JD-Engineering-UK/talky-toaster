
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "audio.h"

#define AUDIO_BUFF_SIZE 2048

static i2s_chan_handle_t tx_chan;

static QueueHandle_t audio_file_queue;

void audio_write_task(void *args);

void audio_init(void)
{
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));

    i2s_std_config_t tx_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(24000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_BCLK_PIN_NUM,
            .dout = CONFIG_DATA_PIN_NUM,
            .din = I2S_GPIO_UNUSED,
            .ws = CONFIG_LRCLK_PIN_NUM,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // tx_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_cfg));
    // printf("Configured audio channel\n");

    audio_file_queue = xQueueCreate(10, sizeof( AudioQueueItem_t ));
    // printf("Created audio file queue\n");

    xTaskCreate(audio_write_task, "audio_write_task", 4096, NULL, 5, NULL);
    // printf("Started audio write task\n");
}

void audio_write_task(void *args)
{
    bool enabled = false;
    while(1)
    {
        AudioQueueItem_t queued_audio;
        if(!xQueueReceive(audio_file_queue, &queued_audio, portMAX_DELAY))
        {
            // printf("Nothing in queue\n");
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        // printf("Got audio file from queue\n");
        size_t w_bytes;
        size_t total_w_bytes = 0;
        if(!enabled)
        {
            ESP_ERROR_CHECK(i2s_channel_preload_data(tx_chan, queued_audio.buf, queued_audio.length, &w_bytes));
            total_w_bytes += w_bytes;
            ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
            enabled = true;
        }

        while(total_w_bytes < queued_audio.length)
        {
            if(i2s_channel_write(
                tx_chan, 
                &queued_audio.buf[total_w_bytes], 
                queued_audio.length - total_w_bytes, 
                &w_bytes, portMAX_DELAY) == ESP_OK
                )
            {
                // printf("Write task: i2s write %d bytes\n", w_bytes);
            }
            total_w_bytes += w_bytes;
        }
        if(enabled && uxQueueMessagesWaiting(audio_file_queue) == 0)
        {
            i2s_channel_disable(tx_chan);
            // printf("I2S channel disabled\n");
            enabled = false;
        }
    }
    vTaskDelete(NULL);
}

void queue_audio(size_t length, const uint8_t *buf)
{   
    AudioQueueItem_t queue_item = {
        .length = length,
        .buf = buf,
    };
    xQueueGenericSend(audio_file_queue, &queue_item, portMAX_DELAY, queueSEND_TO_BACK);
}