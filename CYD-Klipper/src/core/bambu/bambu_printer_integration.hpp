#pragma once

#include "../printer_integration.hpp"
#include <ArduinoJson.h>
#include <WifiClientSecure.h>

class BambuPrinter : public BasePrinter
{
    private:
        unsigned int last_error = 0; 
        unsigned int ignore_error = 0; 
        bool publish_mqtt_command(const char* command);
        unsigned char speed_profile = 2;
        unsigned long print_start;

        union {
            struct {
                bool chamber_light_available : 1;
                bool chamber_light_on : 1;
                bool work_light_available : 1;
                bool work_light_on : 1;
            };
            unsigned char bambu_misc;
        };

    protected:
        void parse_state(JsonDocument& in);
        Files parse_files(WiFiClientSecure& client, int max_files);

    public:
        BambuPrinter(int index) : BasePrinter(index)
        {
            supported_features = PrinterFeatureHome
                | PrinterFeatureDisableSteppers
                | PrinterFeaturePause
                | PrinterFeatureResume
                | PrinterFeatureStop
                | PrinterFeatureEmergencyStop
                | PrinterFeatureCooldown
                | PrinterFeatureContinueError
                | PrinterFeatureExtrude
                | PrinterFeatureRetract
                | PrinterFeatureIgnoreError
                | PrinterFeatureRetryError;

            supported_temperature_devices = PrinterTemperatureDeviceBed 
                | PrinterTemperatureDeviceNozzle1;

            popup_message_timeout_s = -1;
            bambu_misc = 0;
            printer_data.error_screen_features = PrinterFeatureRetryError | PrinterFeatureIgnoreError | PrinterFeatureContinueError;
            print_start = millis();
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
        void receive_data(unsigned char* data, unsigned int length);
};

enum BambuConnectionStatus {
    BambuConnectFail = 0,
    BambuConnectOk = 1,
    BambuConnectSNFail = 2,
};

BambuConnectionStatus connection_test_bambu(PrinterConfiguration* config);