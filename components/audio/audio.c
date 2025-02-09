
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
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_BCLK_PIN_NUM,
            .dout = CONFIG_DATA_PIN_NUM,
            .din = I2S_GPIO_UNUSED,
            .ws = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // tx_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_cfg));
    printf("Configured audio channel\n");

    audio_file_queue = xQueueCreate(10, sizeof( AudioQueueItem_t ));
    printf("Created audio file queue\n");

    xTaskCreate(audio_write_task, "audio_write_task", 4096, NULL, 5, NULL);
    printf("Started audio write task\n");
}

void audio_write_task(void *args)
{
    uint8_t *w_buf = (uint8_t *)calloc(1, AUDIO_BUFF_SIZE);
    assert(w_buf); // Check if w_buf allocation success

    /* Assign w_buf */
    for (int i = 0; i < AUDIO_BUFF_SIZE; i += 8) {
        w_buf[i]     = 0x12;
        w_buf[i + 1] = 0x34;
        w_buf[i + 2] = 0x56;
        w_buf[i + 3] = 0x78;
        w_buf[i + 4] = 0x9A;
        w_buf[i + 5] = 0xBC;
        w_buf[i + 6] = 0xDE;
        w_buf[i + 7] = 0xF0;
    }

    size_t w_bytes = AUDIO_BUFF_SIZE;

    /* (Optional) Preload the data before enabling the TX channel, so that the valid data can be transmitted immediately */
    while (w_bytes == AUDIO_BUFF_SIZE) {
        /* Here we load the target buffer repeatedly, until all the DMA buffers are preloaded */
        ESP_ERROR_CHECK(i2s_channel_preload_data(tx_chan, w_buf, AUDIO_BUFF_SIZE, &w_bytes));
    }

    /* Enable the TX channel */
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    while (1) {
        /* Write i2s data */
        if (i2s_channel_write(tx_chan, w_buf, AUDIO_BUFF_SIZE, &w_bytes, 1000) == ESP_OK) {
            printf("Write Task: i2s write %d bytes\n", w_bytes);
        } else {
            printf("Write Task: i2s write failed\n");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    free(w_buf);
    vTaskDelete(NULL);
}

void queue_audio(uint32_t length, uint32_t *buf)
{   
    AudioQueueItem_t queue_item = {
        .length = length,
        .buf = buf,
    };
    xQueueGenericSend(audio_file_queue, &queue_item, portMAX_DELAY, queueSEND_TO_BACK);
}