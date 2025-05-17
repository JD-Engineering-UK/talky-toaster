#pragma once

#define QUEUE_AUDIO(filename) queue_audio( filename##_start, filename##_end )

typedef struct
{
    const uint8_t *buf_start;
    const uint8_t *buf_end;
} AudioQueueItem_t;

#ifdef __cplusplus
extern "C" {
#endif
void audio_init(void);
void queue_audio(const uint8_t *buf_start, const uint8_t *buf_end);
#ifdef __cplusplus
}
#endif