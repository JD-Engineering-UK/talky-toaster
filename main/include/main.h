#pragma once
#include "sdkconfig.h"
#include <stdio.h>
#include <inttypes.h>
#include "files.h"

#ifdef __cplusplus
extern "C" {
#endif
void initialise_ntp();
void wait_for_updated_time();
#ifdef __cplusplus
}
#endif