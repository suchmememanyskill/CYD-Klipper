#include "../printer_integration.hpp"
#include "klipper_printer_integration.hpp"
#include <ArduinoJson.h>

int KlipperPrinter::parse_slicer_time_estimate(JsonDocument &in)
{
    int time_estimate_s = in["result"]["estimated_time"];
    LOG_F(("Got slicer time estimate: %ds\n", time_estimate_s))
    return time_estimate_s;
}

void KlipperPrinter::parse_state(JsonDocument &in)
{
    JsonObject status = in["result"]["status"];

    if (status.containsKey("webhooks"))
    {
        const char *state = status["webhooks"]["state"];
        const char *message = status["webhooks"]["state_message"];

        if (strcmp(state, "ready") == 0 && printer_data.state == PrinterStateError)
        {
            printer_data.state = PrinterStateIdle;
        }
        else if ((strcmp(state, "shutdown") == 0 || strcmp(state, "error") == 0) && printer_data.state != PrinterStateError)
        {
            printer_data.state = PrinterStateError;
        }

        if (message != NULL && (printer_data.state_message == NULL || strcmp(printer_data.state_message, message)))
        {
            printer_data.state_message = (char *)malloc(strlen(message) + 1);
            strcpy(printer_data.state_message, message);
        }
    }

    if (printer_data.state != PrinterStateError)
    {
        if (status.containsKey("extruder"))
        {
            printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] = status["extruder"]["temperature"];
            printer_data.target_temperatures[PrinterTemperatureDeviceIndexNozzle1] = status["extruder"]["target"];
            bool can_extrude = status["extruder"]["can_extrude"];
            printer_data.pressure_advance = status["extruder"]["pressure_advance"];
            printer_data.smooth_time = status["extruder"]["smooth_time"];
            printer_data.can_extrude = can_extrude == true;
        }

        if (status.containsKey("heater_bed"))
        {
            printer_data.temperatures[PrinterTemperatureDeviceIndexBed] = status["heater_bed"]["temperature"];
            printer_data.target_temperatures[PrinterTemperatureDeviceIndexBed] = status["heater_bed"]["target"];
        }

        if (status.containsKey("toolhead"))
        {
            const char *homed_axis = status["toolhead"]["homed_axes"];
            printer_data.homed_axis = strcmp(homed_axis, "xyz") == 0;
        }

        if (status.containsKey("gcode_move"))
        {
            printer_data.position[0] = status["gcode_move"]["gcode_position"][0];
            printer_data.position[1] = status["gcode_move"]["gcode_position"][1];
            printer_data.position[2] = status["gcode_move"]["gcode_position"][2];
            gcode_offset[0] = status["gcode_move"]["homing_origin"][0];
            gcode_offset[1] = status["gcode_move"]["homing_origin"][1];
            gcode_offset[2] = status["gcode_move"]["homing_origin"][2];
            bool absolute_coords = status["gcode_move"]["absolute_coordinates"];

            if (lock_absolute_relative_mode_swap > 0)
            {
                lock_absolute_relative_mode_swap--;
            }
            else
            {
                printer_data.absolute_coords = absolute_coords == true;
            }

            printer_data.speed_mult = status["gcode_move"]["speed_factor"];
            printer_data.extrude_mult = status["gcode_move"]["extrude_factor"];
            printer_data.feedrate_mm_per_s = status["gcode_move"]["speed"];
            printer_data.feedrate_mm_per_s /= 60; // convert mm/m to mm/s
        }

        if (status.containsKey("fan"))
        {
            printer_data.fan_speed = status["fan"]["speed"];
        }

        if (status.containsKey("virtual_sdcard"))
        {
            printer_data.print_progress = status["virtual_sdcard"]["progress"];
        }

        if (status.containsKey("print_stats"))
        {
            const char *filename = status["print_stats"]["filename"];

            if (filename != NULL && (printer_data.print_filename == NULL || strcmp(printer_data.print_filename, filename)))
            {
                printer_data.print_filename = (char *)malloc(strlen(filename) + 1);
                strcpy(printer_data.print_filename, filename);
            }

            printer_data.elapsed_time_s = status["print_stats"]["total_duration"];
            printer_data.printed_time_s = status["print_stats"]["print_duration"];
            printer_data.filament_used_mm = status["print_stats"]["filament_used"];
            printer_data.total_layers = status["print_stats"]["info"]["total_layer"];
            printer_data.current_layer = status["print_stats"]["info"]["current_layer"];

            const char *state = status["print_stats"]["state"];

            if (state == nullptr)
            {
                // Continue
            }
            else if (strcmp(state, "printing") == 0)
            {
                printer_data.state = PrinterStatePrinting;
            }
            else if (strcmp(state, "paused") == 0)
            {
                printer_data.state = PrinterStatePaused;
            }
            else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
            {
                printer_data.state = PrinterStateIdle;
            }
        }

        if (status.containsKey("display_status"))
        {
            printer_data.print_progress = status["display_status"]["progress"];
            const char *message = status["display_status"]["message"];

            if (message != NULL && (printer_data.popup_message == NULL || strcmp(printer_data.popup_message, message)))
            {
                printer_data.popup_message = (char *)malloc(strlen(message) + 1);
                strcpy(printer_data.popup_message, message);
            }
        }

        if (printer_data.state == PrinterStatePrinting && printer_data.print_progress > 0)
        {
            float remaining_time_s_percentage = (printer_data.printed_time_s / printer_data.print_progress) - printer_data.printed_time_s;
            float remaining_time_s_slicer = 0;

            if (slicer_estimated_print_time_s > 0)
            {
                remaining_time_s_slicer = slicer_estimated_print_time_s - printer_data.printed_time_s;
            }

            if (remaining_time_s_slicer <= 0 || printer_config->remaining_time_calc_mode == REMAINING_TIME_CALC_PERCENTAGE)
            {
                printer_data.remaining_time_s = remaining_time_s_percentage;
            }
            else if (printer_config->remaining_time_calc_mode == REMAINING_TIME_CALC_INTERPOLATED)
            {
                printer_data.remaining_time_s = remaining_time_s_percentage * printer_data.print_progress + remaining_time_s_slicer * (1 - printer_data.print_progress);
            }
            else if (printer_config->remaining_time_calc_mode == REMAINING_TIME_CALC_SLICER)
            {
                printer_data.remaining_time_s = remaining_time_s_slicer;
            }
        }

        if (printer_data.remaining_time_s < 0)
        {
            printer_data.remaining_time_s = 0;
        }

        if (printer_data.state == PrinterStateIdle)
        {
            slicer_estimated_print_time_s = 0;
        }
    }

    if (printer_data.state == PrinterStatePrinting && millis() - last_slicer_time_query > 30000 && slicer_estimated_print_time_s <= 0)
    {
        last_slicer_time_query = millis();
        slicer_estimated_print_time_s = get_slicer_time_estimate_s();
    }
}

void KlipperPrinter::parse_state_min(JsonDocument &in, PrinterDataMinimal* data)
{
    JsonObject status = in["result"]["status"];

    if (status.containsKey("webhooks"))
    {
        const char *state = status["webhooks"]["state"];

        if (strcmp(state, "shutdown") == 0)
        {
            data->state = PrinterState::PrinterStateError;
        }
    }

    if (data->state != PrinterStateError)
    {
        if (status.containsKey("virtual_sdcard"))
        {
            data->print_progress = status["virtual_sdcard"]["progress"];
        }

        if (status.containsKey("print_stats"))
        {
            const char *state = status["print_stats"]["state"];

            if (state == nullptr)
            {
                data->state = PrinterState::PrinterStateError;
            }
            else if (strcmp(state, "printing") == 0)
            {
                data->state = PrinterState::PrinterStatePrinting;
            }
            else if (strcmp(state, "paused") == 0)
            {
                data->state = PrinterState::PrinterStatePaused;
            }
            else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
            {
                data->state = PrinterState::PrinterStateIdle;
            }
        }
    }
}

Macros KlipperPrinter::parse_macros(JsonDocument &in)
{
    JsonObject result = in["result"];
    Macros macros = {0};
    macros.macros = (char **)malloc(sizeof(char *) * 32);
    macros.count = 0;
    macros.success = true;

    for (JsonPair i : result)
    {
        const char *key = i.key().c_str();
        const char *value = i.value().as<String>().c_str();
        if (strcmp(value, "CYD_SCREEN_MACRO") == 0)
        {
            char *macro = (char *)malloc(strlen(key) + 1);
            strcpy(macro, key);
            macros.macros[macros.count++] = macro;
        }
    }

    if (global_config.sort_macros)
    {
        std::sort(macros.macros, macros.macros + macros.count, [](const char *a, const char *b)
                  { return strcmp(a, b) < 0; });
    }

    return macros;
}

int KlipperPrinter::parse_macros_count(JsonDocument &in)
{
    JsonObject result = in["result"];

    int count = 0;

    for (JsonPair i : result)
    {
        const char *value = i.value().as<String>().c_str();
        if (strcmp(value, "CYD_SCREEN_MACRO") == 0)
        {
            count++;
        }
    }

    return count;
}

PowerDevices KlipperPrinter::parse_power_devices(JsonDocument &in)
{
    PowerDevices power_devices = {0};
    JsonArray result = in["result"]["devices"];
    power_devices.power_devices = (char **)malloc(sizeof(char *) * 16);
    power_devices.power_states = (bool *)malloc(sizeof(bool) * 16);
    power_devices.count = 0;
    power_devices.success = true;

    for (JsonObject i : result)
    {
        const char *device_name = i["device"];
        const char *device_state = i["status"];
        power_devices.power_devices[power_devices.count] = (char *)malloc(strlen(device_name) + 1);
        strcpy(power_devices.power_devices[power_devices.count], device_name);
        power_devices.power_states[power_devices.count] = strcmp(device_state, "on") == 0;
        power_devices.count++;
    }

    return power_devices;
}

int KlipperPrinter::parse_power_devices_count(JsonDocument &in)
{
    JsonArray result = in["result"]["devices"];
    int count = 0;

    for (JsonObject i : result)
    {
        count++;
    }

    return count;
}

void KlipperPrinter::parse_file_list(JsonDocument &in, std::list<KlipperFileSystemFile> &files, int fetch_limit)
{
    JsonArray result = in["result"];

    for (JsonObject file : result)
    {
        KlipperFileSystemFile f = {0};
        const char *path = file["path"];
        float modified = file["modified"];
        auto file_iter = files.begin();

        while (file_iter != files.end())
        {
            if ((*file_iter).modified < modified)
                break;

            file_iter++;
        }

        if (file_iter == files.end() && files.size() >= fetch_limit)
            continue;

        f.name = (char *)malloc(strlen(path) + 1);
        if (f.name == NULL)
        {
            LOG_LN("Failed to allocate memory");
            continue;
        }
        strcpy(f.name, path);
        f.modified = modified;

        if (file_iter != files.end())
            files.insert(file_iter, f);
        else
            files.push_back(f);

        if (files.size() > fetch_limit)
        {
            auto last_entry = files.back();

            if (last_entry.name != NULL)
                free(last_entry.name);

            files.pop_back();
        }
    }
}

char *KlipperPrinter::parse_thumbnails(JsonDocument &in)
{
    JsonArray result = in["result"];
    const char *chosen_thumb = NULL;
    for (JsonObject file : result)
    {
        int width = file["width"];
        int height = file["height"];
        int size = file["size"];
        const char *thumbnail = file["thumbnail_path"];

        if (width != height || width != 32)
            continue;

        if (strcmp(thumbnail + strlen(thumbnail) - 4, ".png"))
            continue;

        chosen_thumb = thumbnail;
        break;
    }

    if (chosen_thumb != NULL)
    {
        char* img_filename_path = (char *)malloc(strlen(chosen_thumb) + 1);
        strcpy(img_filename_path, chosen_thumb);
        return img_filename_path;
    }
    
    return NULL;
}