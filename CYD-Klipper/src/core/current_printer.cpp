#include "data_setup.h"
#include "current_printer.h"

bool current_printer_move_printer(const char* axis, float amount, bool relative) 
{
    freeze_request_thread();
    bool result = get_current_printer()->move_printer(axis, amount, relative);
    unfreeze_request_thread();
    return result;
}

bool current_printer_execute_feature(PrinterFeatures feature) 
{
    freeze_request_thread();
    bool result = get_current_printer()->execute_feature(feature);
    unfreeze_request_thread();
    return result;
}

Macros current_printer_get_macros() 
{
    freeze_request_thread();
    Macros macros = get_current_printer()->get_macros();
    unfreeze_request_thread();
    return macros;
}

int current_printer_get_macros_count() 
{
    freeze_request_thread();
    int count = get_current_printer()->get_macros_count();
    unfreeze_request_thread();
    return count;
}

bool current_printer_execute_macro(const char* macro) 
{
    freeze_request_thread();
    bool result = get_current_printer()->execute_macro(macro);
    unfreeze_request_thread();
    return result;
}

PowerDevices current_printer_get_power_devices() 
{
    freeze_request_thread();
    PowerDevices power_devices = get_current_printer()->get_power_devices();
    unfreeze_request_thread();
    return power_devices;
}

int current_printer_get_power_devices_count() 
{
    freeze_request_thread();
    int count = get_current_printer()->get_power_devices_count();
    unfreeze_request_thread();
    return count;
}

bool current_printer_set_power_device_state(const char* device_name, bool state) 
{
    freeze_request_thread();
    bool result = get_current_printer()->set_power_device_state(device_name, state);
    unfreeze_request_thread();
    return result;
}

Files current_printer_get_files() 
{   
    freeze_request_thread();
    Files files = get_current_printer()->get_files();
    unfreeze_request_thread();
    return files;
}

bool current_printer_start_file(const char* filename) 
{
    freeze_request_thread();
    bool result = get_current_printer()->start_file(filename);
    unfreeze_request_thread();
    return result;
}

Thumbnail current_printer_get_32_32_png_image_thumbnail(const char* gcode_filename) 
{
    freeze_request_thread();
    Thumbnail thumbnail = get_current_printer()->get_32_32_png_image_thumbnail(gcode_filename);
    unfreeze_request_thread();
    return thumbnail;
}

bool current_printer_set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature) 
{
    freeze_request_thread();
    bool result = get_current_printer()->set_target_temperature(device, temperature);
    unfreeze_request_thread();
    return result;
}

