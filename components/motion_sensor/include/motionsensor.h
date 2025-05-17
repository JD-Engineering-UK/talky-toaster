#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"


#define BUF_SIZE 1024

// const uart_port_t uart_num = CONFIG_MOTION_SENSOR_UART_NUM;
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t setup_motion_sensor();
uint16_t motion_sensor_get_distance();
bool motion_sensor_get_detected();
#ifdef __cplusplus
}
#endif