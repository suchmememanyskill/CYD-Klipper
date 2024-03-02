#pragma once

typedef struct {
    const char** macros;
    uint32_t count;
} MacrosQueryT;

typedef struct {
    const char** powerDevices;
    const bool* powerStates;
    uint32_t count;
} PowerQueryT;

MacrosQueryT MacrosQuery();
PowerQueryT PowerDevicesQuery();
void MacrosQuerySetup();
bool SetPowerState(const char* deviceName, bool state);
void PowerDevicesQueryInternal();
void PowerDevicesClear();
