#include "include/motionsensor.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "include/module.h"
#include "include/timing.h"
#include "include/report.h"
#include "freertos/semphr.h"


#define UART_PORT (uart_port_t) CONFIG_MOTION_SENSOR_UART_NUM

static bool uart_initialised = false;
static TaskHandle_t* motion_sensor_task_handle;

static motion_sensor_state_t motion_sensor_state = STATE_UNKNOWN;
static TickType_t last_received = 0;
static report_t* report;
static SemaphoreHandle_t mutex = NULL;


uint16_t motion_sensor_get_distance(){
    if(mutex == NULL || report == NULL) return 0;
    uint16_t result;
    xSemaphoreTake(mutex, portMAX_DELAY);
    result = report->distance;
    xSemaphoreGive(mutex);
    return result;
}

bool motion_sensor_get_detected(){
    if(mutex == NULL || report == NULL) return false;
    bool result;
    xSemaphoreTake(mutex, portMAX_DELAY);
    result = report->detected;
    xSemaphoreGive(mutex);
    return result;
}


esp_err_t send_command(uint16_t command_code, uint8_t* command_data,
                       uint16_t command_data_length) {
    uint16_t len = 2 + command_data_length;
    uint8_t buff[10 + len];

    memcpy(buff, COMMAND_HEADER, 4);
    buff[4] = len & 0xFF;
    buff[5] = len >> 8;
    buff[6] = command_code & 0xFF;
    buff[7] = command_code >> 8;
    if (command_data != NULL && command_data_length != 0) {
        memcpy(buff + 8, command_data, command_data_length);
    }
    memcpy(buff + 8 + command_data_length, COMMAND_FOOTER, 4);

    return uart_write_bytes(UART_PORT, buff, 10 + len);
}

esp_err_t set_mode_report() {
    const uint16_t command_code = 0x0012;
    const uint8_t* data = (uint8_t[]){0x00, 0x00, 0x04, 0x00, 0x00, 0x00};
    return send_command(command_code, data, 6);
}

void read_uart(uint8_t* buffer, int* buffer_len, const int max_buf_len,
               TickType_t delay) {
    int len = uart_read_bytes(UART_PORT, buffer + *buffer_len,
                                   max_buf_len - *buffer_len, delay);
    *buffer_len += len;
    if(len > 0) init_timer(&last_received);
}

bool is_report_mode(uint8_t* buffer, int buffer_len) {
    for (int i = 0; i < buffer_len - 4; ++i) {
        if (memcmp(buffer + i, REPORT_HEADER, 4) == 0) return true;
    }
    return false;
}

int find_report_start(uint8_t* buffer, int buffer_len){
    for (int i = 0; i < buffer_len - 4; ++i) {
        if (memcmp(buffer + i, REPORT_HEADER, 4) == 0) return i;
    }
    return -1;
}

bool is_normal_mode(uint8_t* buffer, int buffer_len) {
    for (int i = 0; i < buffer_len - 5; ++i) {
        if (memcmp(buffer + i, "Range", 5) == 0) return true;
    }
    return false;
}



void motion_sensor_task(void* args) {
    uint8_t* buffer = (uint8_t*)malloc(64);
    report = allocate_report();
    bool report_updated = false;
    int buffer_len = 0;
    TickType_t timer = 0;
    int report_offset = -1;
    uint16_t len;
    while (1) {
        read_uart(buffer, &buffer_len, 64, 100 / portTICK_PERIOD_MS);
        report_updated = false;

        switch (motion_sensor_state) {
            case STATE_CONFIGURE:
                if (is_report_mode(buffer, buffer_len)) {
                    motion_sensor_state = STATE_RUNNING;
                    ESP_LOGI(TAG, "Motion sensor configured.");
                    break;
                }
                if (get_timeout(&timer, 500) &&
                    is_normal_mode(buffer, buffer_len)) {
                    ESP_LOGD(
                        TAG,
                        "Motion sensor in normal mode. Setting report mode...");
                    set_mode_report();
                    init_timer(&timer);
                } else if (get_timeout(&timer, 5000)) {
                    ESP_LOGD(TAG, "No mode detected. Setting report mode...");
                    set_mode_report();
                    init_timer(&timer);
                }

                buffer_len = 0;
                break;

            case STATE_RUNNING:
                if(get_timeout(&last_received, 5000)){
                    buffer_len = 0;
                    motion_sensor_state = STATE_CONFIGURE;
                    break;
                }

                // Find the header of the packet in the buffer.
                report_offset = find_report_start(buffer, buffer_len);
                if(report_offset < 0 && buffer_len > 4){
                    // When we don't find the report in the buffer
                    // Discard almost all of the buffer
                    memmove(buffer, buffer+buffer_len-4, 4);
                    buffer_len = 4;
                    break;
                }else if (report_offset < 0)
                {
                    break;
                }
                
                // Found the start of the report
                if(buffer_len-report_offset < 45){
                    // Report length is fixed at 45 bytes.
                    // Discard start of buffer before the report if we don't have the full report in the buffer.
                    memmove(buffer, buffer+report_offset, buffer_len-report_offset);
                    buffer_len -= report_offset;
                    report_offset = -1;
                    break;
                }

                xSemaphoreTake(mutex, portMAX_DELAY);
                parse_report(buffer+report_offset, buffer_len-report_offset, report);
                xSemaphoreGive(mutex);
                report_updated = true;
                if(buffer_len <= 45) buffer_len = 0;
                else{
                    memmove(buffer, buffer+report_offset+45, buffer_len-report_offset-45);
                    buffer_len -= report_offset + 45;
                        report_offset = -1;
                }
                
                break;

            default:
                vTaskDelay(100 / portTICK_PERIOD_MS);
                buffer_len = 0;
                break;
        }

        // if(report_updated)
        // {
        //     ESP_LOGI(TAG, "Detected: %02d, Distance: %02d", report->detected, report->distance);
        //     ESP_LOGI(TAG, "GATES: %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d %02d", report->gate_energies[0], report->gate_energies[1], report->gate_energies[2], report->gate_energies[3], report->gate_energies[4], report->gate_energies[5], report->gate_energies[6], report->gate_energies[7], report->gate_energies[8], report->gate_energies[9], report->gate_energies[10], report->gate_energies[11], report->gate_energies[12], report->gate_energies[13], report->gate_energies[14], report->gate_energies[15]);
        // }
        
    }
    free(buffer);
    deallocate_report(report);
}

esp_err_t setup_motion_sensor() {
    mutex = xSemaphoreCreateMutex();
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t r;
    r = uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "Error installing UART driver");
        return r;
    }
    r = uart_param_config(UART_PORT, &uart_config);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "Error configuring UART");
        return r;
    }
    r = uart_set_pin(UART_PORT, CONFIG_MOTION_SENSOR_UART_TX,
                     CONFIG_MOTION_SENSOR_UART_RX, UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
    if (r != ESP_OK) {
        ESP_LOGE(TAG, "Error setting up pins for UART");
        return r;
    }
    uart_initialised = true;
    ESP_LOGD(TAG, "Motion sensor UART initialised");

    motion_sensor_state = STATE_CONFIGURE;

    // Start the task to receive messages from the sensor
    xTaskCreate(motion_sensor_task, "MotionSensor", 4096, NULL, 1,
                motion_sensor_task_handle);

    return ESP_OK;
}