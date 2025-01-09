#pragma once
#include "printer_integration.hpp"

bool current_printer_move_printer(const char* axis, float amount, bool relative);
bool current_printer_execute_feature(PrinterFeatures feature);
Macros current_printer_get_macros();
int current_printer_get_macros_count();
bool current_printer_execute_macro(const char* macro);
PowerDevices current_printer_get_power_devices();
int current_printer_get_power_devices_count();
bool current_printer_set_power_device_state(const char* device_name, bool state);
Files current_printer_get_files();
bool current_printer_start_file(const char* filename);
Thumbnail current_printer_get_32_32_png_image_thumbnail(const char* gcode_filename);
bool current_printer_set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature);