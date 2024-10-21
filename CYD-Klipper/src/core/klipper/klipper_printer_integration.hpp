#pragma once

#include "../printer_integration.hpp"

class KlipperPrinter : BasePrinter
{
    private:
        unsigned char lock_absolute_relative_mode_swap{};
        unsigned char klipper_request_consecutive_fail_count{};
        unsigned int slicer_estimated_print_time_s{};
        unsigned int last_slicer_time_query{};
        float gcode_offset[3]{};

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

        bool move_printer(const char* axis, float amount, bool relative);
        bool execute_feature(PrinterFeatures feature);
        bool connect();
        bool fetch();
        PrinterDataMinimal fetch_min();
        void disconnect();
        Macros get_macros();
        int get_macros_count();
        bool execute_macro(const char* macro);
        PowerDevices get_power_devices();
        int get_power_devices_count();
        bool set_power_device_state(const char* device_name, bool state);
        Files get_files();
        bool start_file(const char* file);
        bool set_target_temperature(PrinterTemperatureDevice device, float temperature);
        bool send_gcode(const char* gcode, bool wait = true);
        int get_slicer_time_estimate_s();
        void configure_http_client(HTTPClient &client, String url_part, bool stream, int timeout);
};