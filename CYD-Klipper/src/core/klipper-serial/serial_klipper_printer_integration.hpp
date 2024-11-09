#include "../klipper/klipper_printer_integration.hpp"

class SerialKlipperPrinter : public KlipperPrinter
{
    protected:
        bool send_emergency_stop();
        int get_slicer_time_estimate_s();
    public:
        SerialKlipperPrinter(int index) : KlipperPrinter(index)
        {}

        bool connect();
        bool fetch();
        PrinterDataMinimal fetch_min();
        Macros get_macros();
        int get_macros_count();
        PowerDevices get_power_devices();
        int get_power_devices_count();
        bool set_power_device_state(const char* device_name, bool state);
        Files get_files();
        bool start_file(const char* filename);
        Thumbnail get_32_32_png_image_thumbnail(const char* gcode_filename);
        bool send_gcode(const char* gcode, bool wait = true);
};

KlipperConnectionStatus connection_test_serial_klipper(PrinterConfiguration* config);