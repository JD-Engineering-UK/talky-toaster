#pragma once

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

typedef struct 
{
    bool detected;
    uint16_t distance;
    uint16_t* gate_energies;
} report_t;

#ifdef __cplusplus
extern "C" {
#endif

report_t* allocate_report();
void deallocate_report(report_t* report);

void parse_report(uint8_t* buffer, int buffer_len, report_t* report);

#ifdef __cplusplus
}
#endif
