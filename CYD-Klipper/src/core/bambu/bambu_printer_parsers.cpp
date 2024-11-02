#include "bambu_printer_integration.hpp"
#include <HTTPClient.h>

#define BIT_X_AXIS_HOMED BIT(0)
#define BIT_Y_AXIS_HOMED BIT(1)
#define BIT_Z_AXIS_HOMED BIT(2)

void BambuPrinter::parse_state(JsonDocument& in)
{
    if (!in.containsKey("print"))
    {
        return;
    }

    auto print = in["print"];

    if (print.containsKey("print_error"))
    {
        unsigned int error = print["print_error"].as<unsigned int>();
        if (error != last_error)
        {
            last_error = error;

            if (error > 0)
            {
                HTTPClient client;
                client.setTimeout(1000);
                client.setConnectTimeout(1000);

                LOG_F(("Free heap: %d bytes\n", esp_get_free_heap_size()))
    
                char buff[10] = {0};
                sprintf(buff, "%X_%X", error >> 16, error & 0xFFFF);
                int http_status_code = 0;
    
                try 
                {
                    client.begin("http://bambu.suchmeme.nl/" + String(buff));
                    LOG_F(("Sending request to http://bambu.suchmeme.nl/%s", buff));
                    http_status_code = client.GET();
                    LOG_F(("Response: %d", http_status_code));
                }
                catch (...)
                {
                    LOG_LN("Error downloading error code page");
                }
    
                if (http_status_code == 200)
                {
                    printer_data.state_message = (char *)malloc(client.getSize() + 20);
                    sprintf(printer_data.state_message, "%s: %s", buff, client.getString().c_str());
                }
                else 
                {
                    printer_data.state_message = (char *)malloc(20);
                    sprintf(printer_data.state_message, "Error: %s", buff);
                }
            }
        }
    }

    if (print.containsKey("nozzle_temper"))
    {
        printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] = print["nozzle_temper"];
        printer_data.can_extrude = printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] > 175;
    }

    if (print.containsKey("nozzle_target_temper"))
    {
        printer_data.target_temperatures[PrinterTemperatureDeviceIndexNozzle1] = print["nozzle_target_temper"];
    }

    if (print.containsKey("bed_temper"))
    {
        printer_data.temperatures[PrinterTemperatureDeviceIndexBed] = print["bed_temper"];
    }

    if (print.containsKey("bed_target_temper"))
    {
        printer_data.target_temperatures[PrinterTemperatureDeviceIndexBed] = print["bed_target_temper"];
    }

    if (print.containsKey("spd_lvl"))
    {
        speed_profile = print["spd_lvl"];

        switch (speed_profile)
        {
            case 1:
                printer_data.speed_mult = 0.5f;
                break;
            case 2:
                printer_data.speed_mult = 1.0f;
                break;
            case 3:
                printer_data.speed_mult = 1.24f;
                break;
            case 4:
                printer_data.speed_mult = 1.66f;
        }
    }

    if (print.containsKey("home_flag"))
    {
        unsigned int home_flag = print["home_flag"].as<unsigned int>();
        printer_data.homed_axis = (home_flag & (BIT_X_AXIS_HOMED | BIT_Y_AXIS_HOMED | BIT_Z_AXIS_HOMED)) == (BIT_X_AXIS_HOMED | BIT_Y_AXIS_HOMED | BIT_Z_AXIS_HOMED);
    }

    if (last_error > 0 && last_error != ignore_error)
    {
        printer_data.state = PrinterState::PrinterStateError;
    }
    else if (print.containsKey("gcode_state"))
    {
        const char* state = print["gcode_state"];

        if (strcasecmp(state, "pause") == 0)
        {
            printer_data.state = PrinterState::PrinterStatePaused;
        }
        else if (strcasecmp(state, "running") == 0 || strcasecmp(state, "prepare") == 0)
        {
            printer_data.state = PrinterState::PrinterStatePrinting;
        }
        else
        {
            printer_data.state = PrinterState::PrinterStateIdle;
        }
    }

    if (print.containsKey("mc_remaining_time"))
    {
        printer_data.remaining_time_s = print["mc_remaining_time"];
        printer_data.remaining_time_s *= 60;
        if (printer_data.remaining_time_s > 300)
        {
            print_start = millis() - (printer_data.remaining_time_s / (1 - printer_data.print_progress) * printer_data.print_progress * 1000);
        }
    }

    if (print.containsKey("mc_percent"))
    {
        printer_data.print_progress = print["mc_percent"];
        printer_data.print_progress /= 100;
    }

    if (printer_data.state == PrinterState::PrinterStatePrinting)
    {
        printer_data.elapsed_time_s = (millis() - print_start) / 1000;
    }

    if (print.containsKey("layer_num"))
    {
        printer_data.current_layer = print["layer_num"];
    }

    if (print.containsKey("total_layer_num"))
    {
        printer_data.total_layers = print["total_layer_num"];
    }

    if (print.containsKey("lights_report"))
    {
        for (auto lights : print["lights_report"].as<JsonArray>())
        {
            if (lights.containsKey("node") && lights.containsKey("mode"))
            {
                bool mode = !(lights["mode"] == "off");
                const char* node = lights["node"];

                if (node == NULL)
                {
                    continue;
                }

                if (strcmp(node, "chamber_light") == 0)
                {
                    chamber_light_available = true;
                    chamber_light_on = mode;
                }
                else if (strcmp(node, "work_light") == 0)
                {
                    work_light_available = true;
                    work_light_on = mode;
                }
            }
        }
    }

    if (print.containsKey("gcode_file"))
    {
        const char* filename = print["gcode_file"];

        if (filename != NULL && (printer_data.print_filename == NULL || strcmp(printer_data.print_filename, filename)))
        {
            printer_data.print_filename = (char *)malloc(strlen(filename) + 1);
            strcpy(printer_data.print_filename, filename);
        }
    }

    printer_data.extrude_mult = 1;
}

