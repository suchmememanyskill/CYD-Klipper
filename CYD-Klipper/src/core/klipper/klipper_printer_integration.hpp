#pragma once

#include "../printer_integration.hpp"

class KlipperPrinter : BasePrinter
{
    private:
        unsigned char lock_absolute_relative_mode_swap{};

    public:
        KlipperPrinter(int index) : BasePrinter(index)
        {
            supported_features = PrinterFeatureRestart
                | PrinterFeatureFirmwareRestart
                | PrinterFeatureHome
                | PrinterFeatureDisableSteppers
                | PrinterFeaturePause
                | PrinterFeatureResume
                | PrinterFeatureStop
                | PrinterFeatureEmergencyStop
                | PrinterFeatureExtrude
                | PrinterFeatureRetract
                | PrinterFeatureCooldown;

            supported_temperature_devices = PrinterTemperatureDeviceBed 
                | PrinterTemperatureDeviceNozzle1;
        }

        bool move_printer(const char* axis, float amount, bool relative) = 0;
        bool execute_feature(PrinterFeatures feature) = 0;
        bool connect() = 0;
        bool fetch(PrinterData& data) = 0;
        void commit_fetch(PrinterData& data) = 0;
        bool fetch_min(PrinterDataMinimal& data) = 0;
        void disconnect() = 0;
        bool get_macros(Macros& macros) = 0;
        bool execute_macro(const char* macro) = 0;
        bool get_power_devices(PowerDevices& power_devices) = 0;
        bool set_power_device_state(const char* device_name, bool state) = 0;
        bool get_files(Files& files) = 0;
        bool start_file(const char* file) = 0;
        bool set_target_temperature(PrinterTemperatureDevice device, float temperature) = 0;
        bool send_gcode(const char* gcode, bool wait = true);
};