#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool check_latest();
void start_latest_firmware_update();
void start_firmware_update(const char *url);

#ifdef __cplusplus
}
#endif