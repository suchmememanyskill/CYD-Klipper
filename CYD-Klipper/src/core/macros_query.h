#pragma once

#include "../conf/global_config.h"

typedef struct {
    const char** macros;
    uint32_t count;
} MACROSQUERY;

typedef struct {
    const char** power_devices;
    const bool* power_states;
    uint32_t count;
} POWERQUERY;

MACROSQUERY macros_query(PRINTER_CONFIG * config);
MACROSQUERY macros_query();
unsigned int macro_count(PRINTER_CONFIG * config);
unsigned int macro_count();
POWERQUERY power_devices_query(PRINTER_CONFIG * config);
POWERQUERY power_devices_query();
unsigned int power_devices_count(PRINTER_CONFIG * config);
unsigned int power_devices_count();
bool set_power_state(const char* device_name, bool state, PRINTER_CONFIG * config);
bool set_power_state(const char* device_name, bool state);
void macros_clear();
void power_devices_clear();