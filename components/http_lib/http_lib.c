#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include <esp_crt_bundle.h>
#include "http_lib.h"

const char *TAG = "HTTP";

typedef struct {
    char *buffer;
    int buffer_size;
    int data_len;
    http_callback_t callback;
} http_response_t;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len = 0;       // Stores number of bytes read
	int mbedtls_err;
	esp_err_t err;
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			/*
			 *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
			 *  However, event handler can also be used in case chunked encoding is used.
			 */
			if (!esp_http_client_is_chunked_response(evt->client)) {
				// If user_data buffer is configured, copy the response into the buffer
				int copy_len = 0;
				if (evt->user_data && ((http_response_t *)evt->user_data)->buffer != NULL) {
					copy_len = MIN(evt->data_len, (((http_response_t *)evt->user_data)->buffer_size - ((http_response_t *)evt->user_data)->data_len));
					if (copy_len) {
						memcpy(((http_response_t *)evt->user_data)->buffer + ((http_response_t *)evt->user_data)->data_len, evt->data, copy_len);
						((http_response_t *)evt->user_data)->data_len += copy_len;
					}
				} else if (((http_response_t *)evt->user_data)->callback) {
					((http_response_t *)evt->user_data)->callback(evt->data, evt->data_len);
				} else {
					const int buffer_len = esp_http_client_get_content_length(evt->client);
					if (output_buffer == NULL) {
						output_buffer = (char *) malloc(buffer_len);
						output_len = 0;
						if (output_buffer == NULL) {
							ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
							return ESP_FAIL;
						}
					}
					copy_len = MIN(evt->data_len, (buffer_len - output_len));
					if (copy_len) {
						memcpy(output_buffer + output_len, evt->data, copy_len);
					}
				}
				output_len += copy_len;
			}

			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
			mbedtls_err = 0;
			err = esp_http_client_get_and_clear_last_tls_error(evt->client, &mbedtls_err, NULL);
			if (err != 0) {
				ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
				ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			if (output_buffer != NULL) {
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_REDIRECT:
			ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
			esp_http_client_set_header(evt->client, "From", "user@example.com");
			esp_http_client_set_header(evt->client, "Accept", "text/html");
			esp_http_client_set_redirection(evt->client);
			break;
	}
	return ESP_OK;
}


uint32_t http_get(const char *url, char *output_buffer, int output_buffer_size)
{
    http_response_t response = {
        .buffer = output_buffer,
        .buffer_size = output_buffer_size,
        .data_len = 0,
        .callback = NULL
    };

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
		.crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = &response,        // Pass address of local buffer to get response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) 
    {
        int content_length = esp_http_client_get_content_length(client);
        if (content_length < output_buffer_size) 
        {
            esp_http_client_cleanup(client);
            return content_length;
        } 
        else 
        {
            esp_http_client_cleanup(client);
            return 0; // Output buffer too small
        }
    }
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return 0;
}

void http_download(const char *url, http_callback_t callback)
{
    http_response_t response = {
        .buffer = NULL,
        .buffer_size = 0,
        .data_len = 0,
        .callback = callback
    };

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
		.crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = &response,        // Pass address of local buffer to get response
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP GET download failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}
