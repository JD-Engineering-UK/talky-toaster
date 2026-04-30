#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*http_callback_t)(const char *data, int len);

uint32_t http_get(const char *url, char *output_buffer, int output_buffer_size);
void http_download(const char *url, http_callback_t callback);

#ifdef __cplusplus
}
#endif