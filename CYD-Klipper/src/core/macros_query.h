#pragma once

typedef struct {
    const char** macros;
    uint32_t count;
} MacrosQuery;

typedef struct {
    const char** powerDevices;
    const bool* powerStates;
    uint32_t count;
} PowerQuery;

MacrosQuery macrosQuery();
PowerQuery powerDevicesQuery();
void macrosQuerySetup();
bool setPowerState(const char* deviceName, bool state);
void powerDevicesQueryInternal();
void powerDevicesClear();
