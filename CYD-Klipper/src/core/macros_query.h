#pragma once

typedef struct {
    const char** macros;
    uint32_t count;
} MACROSQUERY;

typedef struct {
    const char** power_devices;
    const bool* power_states;
    uint32_t count;
} POWERQUERY;

MACROSQUERY macros_query();
POWERQUERY power_devices_query();
void macros_query_setup();
bool set_power_state(const char* device_name, bool state);
void _power_devices_query_internal();
void _macros_query_internal();
void power_devices_clear();