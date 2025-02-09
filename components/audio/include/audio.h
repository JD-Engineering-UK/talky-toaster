#pragma once

typedef struct
{
    size_t length;
    const uint8_t *buf;
} AudioQueueItem_t;

void audio_init(void);
void queue_audio(size_t length, const uint8_t *buf);