#include "../printer_integration.hpp"
#include <HTTPClient.h>
#include <ArduinoJson.h>

class OctoPrinter : public BasePrinter
{
    protected:
        bool make_request(const char* endpoint, HttpRequestType requestType = HttpRequestType::HttpGet, int timeout_ms = 1000, bool stream = true);
        bool make_request(JsonDocument& doc, const char* endpoint, HttpRequestType requestType = HttpRequestType::HttpGet, int timeout_ms = 1000, bool stream = true);

    public:
        OctoPrinter(int index) : BasePrinter(index)
        {
            supported_features = PrinterFeatureHome
                | PrinterFeatureDisableSteppers
                | PrinterFeaturePause
                | PrinterFeatureResume
                | PrinterFeatureStop
                | PrinterFeatureExtrude
                | PrinterFeatureRetract
                | PrinterFeatureCooldown
                | PrinterFeatureRetryError;

            supported_temperature_devices = PrinterTemperatureDeviceBed 
                | PrinterTemperatureDeviceNozzle1;

            printer_data.error_screen_features = PrinterFeatureRetryError;
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
};

enum OctoConnectionStatus {
    OctoConnectFail = 0,
    OctoConnectOk = 1,
    OctoConnectKeyFail = 2,
};

OctoConnectionStatus connection_test_octoprint(PrinterConfiguration* config);