
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "audio.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "board.h"



#define AUDIO_BUFF_SIZE 2048

static const char *TAG = "AUDIO";
void audio_write_task(void *args);



static QueueHandle_t audio_file_queue;

audio_pipeline_handle_t pipeline;
audio_element_handle_t i2s_stream_writer, mp3_decoder;
int player_volume;
audio_event_iface_handle_t evt;


static struct marker {
    int pos;
    const uint8_t *start;
    const uint8_t *end;
} file_marker;

int mp3_music_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
{
    int read_size = file_marker.end - file_marker.start - file_marker.pos;
    if (read_size == 0) {
        return AEL_IO_DONE;
    } else if (len < read_size) {
        read_size = len;
    }
    memcpy(buf, file_marker.start + file_marker.pos, read_size);
    file_marker.pos += read_size;
    return read_size;
}



void audio_init(void)
{
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
    audio_hal_get_volume(board_handle->audio_hal, &player_volume);
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);
    audio_element_set_read_cb(mp3_decoder, mp3_music_read_cb, NULL);
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");
    const char *link_tag[2] = {"mp3", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 2);
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);

    audio_pipeline_set_listener(pipeline, evt);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);



    audio_file_queue = xQueueCreate(10, sizeof( AudioQueueItem_t ));

    xTaskCreate(audio_write_task, "audio_write_task", 4096, NULL, 5, NULL);
}

void audio_write_task(void *args)
{
    while(1)
    {
        AudioQueueItem_t queued_audio;
        if(!xQueueReceive(audio_file_queue, &queued_audio, portMAX_DELAY))
        {
            // printf("Nothing in queue\n");
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        printf("Got audio file from queue\n");

        file_marker.start = queued_audio.buf;
        file_marker.end = queued_audio.buf + queued_audio.length;
        file_marker.pos = 0;
        
        audio_pipeline_run(pipeline);
        
        while(1)
        {
            audio_event_iface_msg_t msg;
            esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
            if (ret != ESP_OK) {
                continue;
            }

            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) mp3_decoder
                && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
                audio_element_info_t music_info = {0};
                audio_element_getinfo(mp3_decoder, &music_info);
                ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                        music_info.sample_rates, music_info.bits, music_info.channels);
                i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
                continue;
            }
            audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
            if(el_state == AEL_STATE_FINISHED)
            {
                ESP_LOGI(TAG,"Audio clip ended");
                // audio_pipeline_stop(pipeline);
                // audio_pipeline_wait_for_stop(pipeline);
                audio_pipeline_reset_ringbuffer(pipeline);
                audio_pipeline_reset_elements(pipeline);
                audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
                break;
            }
            ESP_LOGI(TAG, "Event %d", (int)msg.cmd);
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