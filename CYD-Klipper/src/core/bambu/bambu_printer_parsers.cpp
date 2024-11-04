#include "bambu_printer_integration.hpp"
#include <HTTPClient.h>
#include <list>

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

// Derived from https://github.com/ldab/ESP32_FTPClient/blob/master/src/ESP32_FTPClient.cpp
bool wifi_client_response_pass(WiFiClientSecure& client)
{
    unsigned long _m = millis();
    bool first_char = true;
    while (!client.available() && millis() < _m + 500) delay(1);   

    if(!client.available())
    {
        LOG_LN("FTPS: No response from server");

        return false;
    }

    LOG_LN("[FTPS response]");
    bool response = true;
    while (client.available()) 
    {
        char byte = client.read();

        if (first_char && (byte == '4' || byte == '5'))
        {
            LOG_LN("FTPS: Server returned an error");
            response = false;
        }

        first_char = false;

        LOG_F(("%c", byte));
    }

    return response;
}

bool wifi_client_response_parse(WiFiClientSecure& client, std::list<char*> &files, int max_files)
{
    unsigned long _m = millis();
    while (!client.available() && millis() < _m + 500) delay(1);   

    if(!client.available())
    {
        LOG_LN("FTPS: No response from server");
        return false;
    }

    LOG_LN("[FTPS response]");
    char buff[128] = {0};
    int index = 0;
    while (client.available()) {
        int byte = client.read();
        LOG_F(("%c", byte));
        buff[index] = byte;

        if (byte == '\n' || byte == '\r' || byte <= 0)
        {
            buff[index] = 0;
            if (index > 10)
            {
                char* file = (char*)malloc(index + 1);

                if (file != NULL)
                {
                    strcpy(file, buff);
                    files.push_front(file);

                    if (files.size() > max_files)
                    {
                        auto last_entry = files.back();

                        if (last_entry != NULL)
                            free(last_entry);

                        files.pop_back();
                    }
                }
                else 
                {
                    LOG_LN("Failed to allocate memory");
                }
            }

            index = 0;
        }
        else 
        {
            index++;
        }
    }

    return true;
}

bool send_command_without_response(WiFiClientSecure& client, const char* command)
{
    client.println(command);
    LOG_F(("[FTPS Command] %s\n", command));
    return wifi_client_response_pass(client);
}

Files BambuPrinter::parse_files(WiFiClientSecure& wifi_client, int max_files)
{
    LOG_F(("Heap space pre-file-parse: %d bytes\n", esp_get_free_heap_size()));

    unsigned long timer_request = millis();
    Files result = {0};

    if (!wifi_client.connect(printer_config->klipper_host, 990))
    {
        LOG_LN("Failed to fetch files: connection failed");
    }

    wifi_client_response_pass(wifi_client);
    
    char auth_code_buff[16] = {0};
    sprintf(auth_code_buff, "PASS %d", printer_config->klipper_port);
    send_command_without_response(wifi_client, "USER bblp");
    wifi_client_response_pass(wifi_client);
    send_command_without_response(wifi_client, auth_code_buff);
    send_command_without_response(wifi_client, "PASV");
    send_command_without_response(wifi_client, "NLST");
    wifi_client.stop();

    if (wifi_client.connect(printer_config->klipper_host, 2024))
    {
        unsigned long timer_parse = millis();
        std::list<char*> files;
        wifi_client_response_parse(wifi_client, files, max_files);
        result.available_files = (char**)malloc(sizeof(char*) * files.size());
        if (result.available_files == NULL)
        {
            LOG_LN("Failed to allocate memory");

            for (auto file : files){
                free(file);
            }

            return result;
        }

        for (auto file : files){
            result.available_files[result.count++] = file;
        }

        result.success = true;
        LOG_F(("Heap space post-file-parse: %d bytes\n", esp_get_free_heap_size()))
        LOG_F(("Got %d files. Request took %dms, parsing took %dms\n", files.size(), timer_parse - timer_request, millis() - timer_parse))
    }   
    else 
    {
        LOG_LN("Failed to fetch files: data connection failed");
    }

    wifi_client.stop();
    return result;
}