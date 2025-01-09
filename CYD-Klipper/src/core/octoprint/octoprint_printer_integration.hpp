#include "../printer_integration.hpp"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <list>

typedef struct {
    char* name;
    float modified;
} OctoFileSystemFile;

class OctoPrinter : public BasePrinter
{
    protected:
        bool no_printer = false;
        unsigned char request_consecutive_fail_count{};

        void parse_printer_status(JsonDocument& in);
        PrinterState parse_printer_state(JsonDocument& in);
        void parse_job_state(JsonDocument& in);
        float parse_job_state_progress(JsonDocument& in);
        void parse_error(JsonDocument& in);
        void parse_file_list(JsonDocument &in, std::list<OctoFileSystemFile> &files, int fetch_limit);

        bool get_request(const char* endpoint, int timeout_ms = 1000);
        void init_ui_panels();

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

            init_ui_panels();
        }

        bool post_request(const char* endpoint, const char* body, int timeout_ms = 1000);
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

enum OctoConnectionStatus {
    OctoConnectFail = 0,
    OctoConnectOk = 1,
    OctoConnectKeyFail = 2,
};

OctoConnectionStatus connection_test_octoprint(PrinterConfiguration* config);