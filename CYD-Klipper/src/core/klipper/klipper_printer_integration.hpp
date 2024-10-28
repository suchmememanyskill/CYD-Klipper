#pragma once

#include "../printer_integration.hpp"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <list>

typedef struct {
    char* name;
    float modified;
} FileSystemFile;

class KlipperPrinter : public BasePrinter
{
    private:
        unsigned char lock_absolute_relative_mode_swap{};
        unsigned char klipper_request_consecutive_fail_count{};
        unsigned int slicer_estimated_print_time_s{};
        unsigned int last_slicer_time_query{};
        void configure_http_client(HTTPClient &client, String url_part, bool stream, int timeout);

    protected:
        bool send_emergency_stop();
        int get_slicer_time_estimate_s();
        void init_ui_panels();

        int parse_slicer_time_estimate(JsonDocument& in);
        void parse_state(JsonDocument& in);
        PrinterDataMinimal parse_state_min(JsonDocument& in);
        Macros parse_macros(JsonDocument &in);
        int parse_macros_count(JsonDocument &in);
        PowerDevices parse_power_devices(JsonDocument &in);
        int parse_power_devices_count(JsonDocument &in);
        void parse_file_list(JsonDocument &in, std::list<FileSystemFile> &files, int fetch_limit);
        char *parse_thumbnails(JsonDocument &in);

    public:
        float gcode_offset[3]{};

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

            init_ui_panels();
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
        bool start_file(const char* filename);
        Thumbnail get_32_32_png_image_thumbnail(const char* gcode_filename);
        bool set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature);
        bool send_gcode(const char* gcode, bool wait = true);
};

enum ConnectionStatus {
    ConnectFail = 0,
    ConnectOk = 1,
    ConnectAuthRequired = 2,
};

ConnectionStatus connection_test_klipper(PrinterConfiguration* config);