#include "klipper_printer_integration.hpp"
#include "../../conf/global_config.h"
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ArduinoJson.h>

void KlipperPrinter::configure_http_client(HTTPClient &client, String url_part, bool stream, int timeout)
{
    if (stream){
        client.useHTTP10(true);
    }

    if (timeout > 0){
        client.setTimeout(timeout);
        client.setConnectTimeout(timeout);
    }

    client.begin("http://" + String(printer_config->klipper_host) + ":" + String(printer_config->klipper_port) + url_part);

    if (printer_config->auth_configured) {
        client.addHeader("X-Api-Key", printer_config->klipper_auth);
    }
}

int KlipperPrinter::get_slicer_time_estimate_s()
{
    if (printer_data.state != PrinterStatePrinting && printer_data.state != PrinterStatePaused)
        return 0;

    HTTPClient client;
    configure_http_client(client, "/server/files/metadata?filename=" + urlEncode(printer_data.print_filename), true, 5000);
    int httpCode = client.GET();

    if (httpCode != 200) 
        return 0;
    
    JsonDocument doc;
    deserializeJson(doc, client.getStream());
    int time_estimate_s = doc["result"]["estimated_time"];
    LOG_F(("Got slicer time estimate: %ds\n", time_estimate_s))
    return time_estimate_s;
}

bool KlipperPrinter::send_gcode(const char *gcode, bool wait)
{
    HTTPClient client;
    configure_http_client(client, "/printer/gcode/script?script=" + urlEncode(gcode), false, wait ? 5000 : 750);
    LOG_F(("Sending gcode: %s\n", gcode))

    try
    {
        client.GET();
        return true;
    }
    catch (...)
    {
        LOG_LN("Failed to send gcode");
        return false;
    }
}

bool KlipperPrinter::move_printer(const char* axis, float amount, bool relative)
{
    if (!printer_data.homed_axis || printer_data.state == PrinterStatePrinting)
        return true;

    char gcode[64];
    const char* extra = (amount > 0) ? "+" : "";
    const char* start = "";
    const char* end = "";

    if (printer_data.absolute_coords && relative) {
        start = "G91\n";
    }
    else if (!printer_data.absolute_coords && !relative) {
        start = "G90\n";
    }

    if (printer_data.absolute_coords && relative) {
        end = "\nG90";
    }
    else if (!printer_data.absolute_coords && !relative) {
        end = "\nG91";
    }

    sprintf(gcode, "%sG1 %s%s%.3f F6000%s", start, axis, extra, amount, end);
    send_gcode(gcode);

    lock_absolute_relative_mode_swap = 2;
}

bool KlipperPrinter::execute_feature(PrinterFeatures feature)
{
    HTTPClient client;

    switch (feature)
    {
        case PrinterFeatureRestart:
            return send_gcode("RESTART", false);
        case PrinterFeatureFirmwareRestart:
            return send_gcode("FIRMWARE_RESTART", false);
        case PrinterFeatureHome:
            return send_gcode("G28");
        case PrinterFeatureDisableSteppers:
            return send_gcode("M18");
        case PrinterFeaturePause:
            return send_gcode("PAUSE");
        case PrinterFeatureResume:
            return send_gcode("RESUME");
        case PrinterFeatureStop:
            return send_gcode("CANCEL_PRINT");
        case PrinterFeatureEmergencyStop:
            LOG_LN("Sending estop");
            send_gcode("M112", false);
            configure_http_client(client, "/printer/emergency_stop", false, 5000);

            try
            {
                client.GET();
            }
            catch (...)
            {
                LOG_LN("Failed to send estop");
            }

            return true;
        case PrinterFeatureExtrude:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            if (printer_config->custom_filament_move_macros)
            {
                return send_gcode("FILAMENT_EXTRUDE");
            }
            else 
            {
                return send_gcode("M83\nG1 E25 F300");
            }
        case PrinterFeatureRetract:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            if (get_current_printer_config()->custom_filament_move_macros)
            {
                return send_gcode("FILAMENT_RETRACT");
            }
            else 
            {
                return send_gcode("M83\nG1 E-25 F300");
            }
        case PrinterFeatureCooldown:
            return send_gcode("M104 S0\nM140 S0");
        default:
            LOG_F(("Unsupported printer feature %d", feature));
            return false;
    }
}

bool KlipperPrinter::connect()
{
    HTTPClient client;
    configure_http_client(client, "/printer/info", false, 1000);

    int httpCode;
    try {
        httpCode = client.GET();
        return httpCode == 200;
    }
    catch (...) {
        LOG_LN("Failed to connect");
        return false;
    }
}

bool KlipperPrinter::fetch()
{
    HTTPClient client;
    configure_http_client(client, "/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan&display_status", true, 1000);

    int httpCode = client.GET();
    if (httpCode == 200)
    {
        if (printer_data.state == PrinterStateOffline)
        {
            printer_data.state = PrinterStateError;
        }

        klipper_request_consecutive_fail_count = 0;
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto status = doc["result"]["status"];
        bool emit_state_update = false;
        
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

            if (printer_data.state_message == NULL || strcmp(printer_data.state_message, message))
            {
                if (printer_data.state_message != NULL)
                {
                    free(printer_data.state_message);
                }

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
                    if (printer_data.print_filename != NULL)
                    {
                        free(printer_data.print_filename);
                    }

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
                const char* message = status["display_status"]["message"];
                store_available_popup_message(message);
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
    else
    {
        klipper_request_consecutive_fail_count++;
        LOG_F(("Failed to fetch printer data: %d\n", httpCode));

        if (klipper_request_consecutive_fail_count >= 5) 
        {
            printer_data.state = PrinterStateOffline;
            return false;
        }
    }

    return true;
}

int KlipperPrinter::get_power_devices_count()
{
    return 0;
}

PrinterDataMinimal KlipperPrinter::fetch_min()
{
    PrinterDataMinimal data = {0};

    if (!printer_config->ip_configured)
    {
        data.state = PrinterStateOffline;
        return data;
    }

    data.success = true;

    HTTPClient client;
    configure_http_client(client, "/printer/objects/query?webhooks&print_stats&virtual_sdcard", true, 1000);

    int httpCode = client.GET();
    if (httpCode == 200)
    {
        data.state = PrinterStateIdle;
        data.power_devices = get_power_devices_count();

        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto status = doc["result"]["status"];

        if (status.containsKey("webhooks"))
        {
            const char *state = status["webhooks"]["state"];

            if (strcmp(state, "shutdown") == 0)
            {
                data.state = PrinterStateError;
            }
        }

        if (data.state != PrinterStateError)
        {
            if (status.containsKey("virtual_sdcard"))
            {
                data.print_progress = status["virtual_sdcard"]["progress"];
            }

            if (status.containsKey("print_stats"))
            {
                const char *state = status["print_stats"]["state"];

                if (state == nullptr)
                {
                    data.state = PrinterStateError;
                }
                else if (strcmp(state, "printing") == 0)
                {
                    data.state = PrinterStatePrinting;
                }
                else if (strcmp(state, "paused") == 0)
                {
                    data.state = PrinterStatePaused;
                }
                else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
                {
                    data.state = PrinterStateIdle;
                }
            }
        }
    }
    else 
    {
        data.state = PrinterStateOffline;
        data.power_devices = get_power_devices_count();
    }
}