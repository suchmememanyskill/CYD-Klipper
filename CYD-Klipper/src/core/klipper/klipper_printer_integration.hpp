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
        unsigned int slicer_estimated_print_time_s{};
        unsigned int last_slicer_time_query{};
        void configure_http_client(HTTPClient &client, String url_part, bool stream, int timeout);

    protected:
        unsigned char lock_absolute_relative_mode_swap{};
        unsigned char klipper_request_consecutive_fail_count{};

        bool send_emergency_stop();
        int get_slicer_time_estimate_s();
        void init_ui_panels();

        int parse_slicer_time_estimate(JsonDocument& in);
        void parse_state(JsonDocument& in);
        void parse_state_min(JsonDocument &in, PrinterDataMinimal* data);
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
            printer_data.error_screen_features = PrinterFeatureRestart | PrinterFeatureFirmwareRestart;
        }

        bool move_printer(const char* axis, float amount, bool relative);
        bool execute_feature(PrinterFeatures feature);
        virtual bool connect();
        virtual bool fetch();
        virtual PrinterDataMinimal fetch_min();
        void disconnect();
        virtual Macros get_macros();
        virtual int get_macros_count();
        bool execute_macro(const char* macro);
        virtual PowerDevices get_power_devices();
        virtual int get_power_devices_count();
        virtual bool set_power_device_state(const char* device_name, bool state);
        virtual Files get_files();
        virtual bool start_file(const char* filename);
        virtual Thumbnail get_32_32_png_image_thumbnail(const char* gcode_filename);
        bool set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature);
        virtual bool send_gcode(const char* gcode, bool wait = true);
};

enum KlipperConnectionStatus {
    ConnectFail = 0,
    ConnectOk = 1,
    ConnectAuthRequired = 2,
};

KlipperConnectionStatus connection_test_klipper(PrinterConfiguration* config);