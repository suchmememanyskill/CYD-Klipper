#pragma once
#include "../conf/global_config.h"

#define BIT(x) 1 << x

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
    PrinterFeatureLoadFilament = BIT(10),
    PrinterFeatureUnloadFilament = BIT(11),
    PrinterFeatureCooldown = BIT(12),
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
        PrinterTemperatureDevice AvailableDevices;
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
} PrinterData;

typedef struct  {
    unsigned char state;
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
    const char** available_files;
    unsigned int count;
    bool success;
} Files;

typedef struct {
    void* set_label;  // type lv_event_cb_t
    void* open_panel; // type lv_event_cb_t
} PrinterUiPanel;

class BasePrinter 
{
    protected:
        unsigned char config_index{};
    
    public:
        PrinterData printer_data{};
        PrinterFeatures supported_features{};
        PrinterTemperatureDevice supported_temperature_devices{};
        PrinterUiPanel* custom_menus{};
        PRINTER_CONFIG* printer_config{};
        GLOBAL_CONFIG* global_config{};
        unsigned char custom_menus_count{};

        virtual bool move_printer(const char* axis, float amount, bool relative) = 0;
        virtual bool execute_feature(PrinterFeatures feature) = 0;
        virtual bool connect() = 0;
        virtual bool fetch() = 0;
        virtual PrinterDataMinimal fetch_min() = 0;
        virtual void disconnect() = 0;
        virtual Macros get_macros() = 0;
        virtual int get_macros_count() = 0;
        virtual bool execute_macro(const char* macro) = 0;
        virtual PowerDevices get_power_devices() = 0;
        virtual int get_power_devices_count() = 0;
        virtual bool set_power_device_state(const char* device_name, bool state) = 0;
        virtual Files get_files() = 0;
        virtual bool start_file(const char* filename) = 0;
        virtual unsigned char* get_32_32_png_image_thumbnail(const char* gcode_filename);
        virtual bool set_target_temperature(PrinterTemperatureDevice device, float temperature) = 0;

        BasePrinter(unsigned char index);
        PrinterData* AnnouncePrinterData();
};

BasePrinter* get_current_printer();
BasePrinter* get_printer(int idx);
void initialize_printers();