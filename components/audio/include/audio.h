#pragma once

typedef struct
{
    uint32_t length;
    uint32_t *buf;
} AudioQueueItem_t;

void audio_init(void);
void queue_audio(uint32_t length, uint32_t *buf);