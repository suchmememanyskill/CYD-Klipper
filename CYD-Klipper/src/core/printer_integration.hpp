#pragma once
#include "../conf/global_config.h"
#include <esp_task_wdt.h>

#define MIN_EXTRUDER_EXTRUDE_TEMP 175

enum PrinterFeatures {
    PrinterFeatureRestart = BIT(0),
    PrinterFeatureFirmwareRestart = BIT(1),
    PrinterFeatureHome = BIT(2),
    PrinterFeatureDisableSteppers = BIT(3),
    PrinterFeaturePause = BIT(4),
    PrinterFeatureResume = BIT(5),
    PrinterFeatureStop = BIT(6),
    PrinterFeatureEmergencyStop = BIT(7),
    PrinterFeatureExtrude = BIT(8),
    PrinterFeatureRetract = BIT(9),
    PrinterFeatureIgnoreError = BIT(10),
    PrinterFeatureContinueError = BIT(11),
    PrinterFeatureCooldown = BIT(12),
    PrinterFeatureRetryError = BIT(13),
};

inline PrinterFeatures operator|(PrinterFeatures a, PrinterFeatures b)
{
    return static_cast<PrinterFeatures>(static_cast<int>(a) | static_cast<int>(b));
}

enum PrinterTemperatureDevice
{
    PrinterTemperatureDeviceBed = BIT(0),
    PrinterTemperatureDeviceNozzle1 = BIT(1),
    PrinterTemperatureDeviceNozzle2 = BIT(2),
    PrinterTemperatureDeviceNozzle3 = BIT(3),
    PrinterTemperatureDeviceNozzle4 = BIT(4),
    PrinterTemperatureDeviceNozzle5 = BIT(5),
    PrinterTemperatureDeviceNozzle6 = BIT(6),
    PrinterTemperatureDeviceNozzle7 = BIT(7),
    PrinterTemperatureDeviceNozzle8 = BIT(8),
    PrinterTemperatureDeviceChamber = BIT(9),
};

enum PrinterTemperatureDeviceIndex
{
    PrinterTemperatureDeviceIndexBed = 0,
    PrinterTemperatureDeviceIndexNozzle1 = 1,
    PrinterTemperatureDeviceIndexNozzle2 = 2,
    PrinterTemperatureDeviceIndexNozzle3 = 3,
    PrinterTemperatureDeviceIndexNozzle4 = 4,
    PrinterTemperatureDeviceIndexNozzle5 = 5,
    PrinterTemperatureDeviceIndexNozzle6 = 6,
    PrinterTemperatureDeviceIndexNozzle7 = 7,
    PrinterTemperatureDeviceIndexNozzle8 = 8,
    PrinterTemperatureDeviceIndexChamber = 9,
};

inline PrinterTemperatureDevice operator|(PrinterTemperatureDevice a, PrinterTemperatureDevice b)
{
    return static_cast<PrinterTemperatureDevice>(static_cast<int>(a) | static_cast<int>(b));
}

enum PrinterState {
    PrinterStateOffline = 0,
    PrinterStateError = 1,
    PrinterStateIdle = 2,
    PrinterStatePrinting = 3,
    PrinterStatePaused = 4,
};

typedef struct _PrinterData {
        union {
            struct {
                bool can_extrude : 1;
                bool homed_axis : 1;
                bool absolute_coords : 1;
            };
            unsigned char rawState;
        };
        PrinterState state;
        char* state_message;
        char* popup_message;
        float temperatures[10];
        float target_temperatures[10];
        float position[3];
        float elapsed_time_s;
        float printed_time_s;
        float remaining_time_s;
        float filament_used_mm;
        char* print_filename; 
        float print_progress; // 0 -> 1
        float fan_speed; // 0 -> 1
        float speed_mult;
        float extrude_mult;
        int total_layers;
        int current_layer;
        float pressure_advance;
        float smooth_time;
        int feedrate_mm_per_s;
        PrinterFeatures error_screen_features;
} PrinterData;

typedef struct {
    PrinterState state;
    float print_progress; // 0 -> 1
    unsigned int power_devices;
    bool success;
} PrinterDataMinimal;

typedef struct {
    char** macros;
    unsigned int count;
    bool success;
} Macros;

typedef struct {
    char** power_devices;
    bool* power_states;
    unsigned int count;
    bool success;
} PowerDevices;

typedef struct {
    char** available_files;
    unsigned int count;
    bool success;
} Files;

typedef struct {
    void* set_label;  // type lv_event_cb_t
    void* open_panel; // type lv_event_cb_t
} PrinterUiPanel;

typedef struct {
    bool success;
    unsigned int size;
    unsigned char* png;
} Thumbnail;

enum HttpRequestType
{
    HttpPost,
    HttpGet
};

class BasePrinter 
{
    protected:
        unsigned char config_index{};
        PrinterData printer_data{};
        
    public:
        short popup_message_timeout_s = 10;

        PrinterConfiguration* printer_config{};
        PrinterFeatures supported_features{};
        PrinterTemperatureDevice supported_temperature_devices{};
        PrinterUiPanel* custom_menus{};
        unsigned char custom_menus_count{};

        virtual bool move_printer(const char* axis, float amount, bool relative) = 0;
        virtual bool execute_feature(PrinterFeatures feature) = 0;
        virtual bool connect() = 0;
        virtual bool fetch() = 0;
        virtual PrinterDataMinimal fetch_min() = 0;
        virtual void disconnect() = 0;
        // Free macros externally when done
        virtual Macros get_macros() = 0;
        virtual int get_macros_count() = 0;
        virtual bool execute_macro(const char* macro) = 0;
        // Free power devices externally when done
        virtual PowerDevices get_power_devices() = 0;
        virtual int get_power_devices_count() = 0;
        virtual bool set_power_device_state(const char* device_name, bool state) = 0;
        // Free files externally when done
        virtual Files get_files() = 0;
        virtual bool start_file(const char* filename) = 0;
        // Free thumbnail externally when done
        virtual Thumbnail get_32_32_png_image_thumbnail(const char* gcode_filename) = 0;
        virtual bool set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature) = 0;

        BasePrinter(unsigned char index);
        PrinterData* AnnouncePrinterData();
};

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2
#define DATA_PRINTER_TEMP_PRESET 3
#define DATA_PRINTER_MINIMAL 4
#define DATA_PRINTER_POPUP 5

BasePrinter* get_current_printer();
BasePrinter* get_printer(int idx);
void initialize_printers(BasePrinter** printers, unsigned char total);
PrinterData* get_current_printer_data();
unsigned int get_printer_count();
void announce_printer_data_minimal(PrinterDataMinimal* printer_data);
PrinterDataMinimal* get_printer_data_minimal(int idx);
int get_current_printer_index();
void set_current_printer(int idx);