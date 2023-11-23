
#include "data_setup.h"
#include "lvgl.h"
#include "../conf/global_config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "macros_query.h"

const char *printer_state_messages[] = {
    "Error",
    "Idle",
    "Printing"};

Printer printer = {0};

void send_gcode(bool wait, const char *gcode)
{
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/printer/gcode/script?script=%s", global_config.klipperHost, global_config.klipperPort, gcode);
    HTTPClient client;
    client.begin(buff);

    if (!wait)
    {
        client.setTimeout(1000);
    }

    try
    {
        client.GET();
    }
    catch (...)
    {
        Serial.println("Failed to send gcode");
    }
}

char filename_buff[512] = {0};

void fetch_printer_data()
{
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks", global_config.klipperHost, global_config.klipperPort);
    HTTPClient client;
    client.begin(buff);
    int httpCode = client.GET();
    if (httpCode == 200)
    {
        String payload = client.getString();
        DynamicJsonDocument doc(4096);
        deserializeJson(doc, payload);
        auto status = doc["result"]["status"];
        bool emit_state_update = false;
        int printer_state = printer.state;

        if (status.containsKey("webhooks"))
        {
            const char *state = status["webhooks"]["state"];
            const char *message = status["webhooks"]["state_message"];

            if (strcmp(state, "ready") == 0 && printer.state == PRINTER_STATE_ERROR)
            {
                printer_state = PRINTER_STATE_IDLE;
            }
            else if (strcmp(state, "shutdown") == 0 && printer.state != PRINTER_STATE_ERROR)
            {
                printer_state = PRINTER_STATE_ERROR;
            }

            if (printer.state_message == NULL || strcmp(printer.state_message, message))
            {
                if (printer.state_message != NULL)
                {
                    free(printer.state_message);
                }

                printer.state_message = (char *)malloc(strlen(message) + 1);
                strcpy(printer.state_message, message);
                emit_state_update = true;
            }
        }

        if (printer_state != PRINTER_STATE_ERROR)
        {
            if (status.containsKey("extruder"))
            {
                printer.extruder_temp = status["extruder"]["temperature"];
                printer.extruder_target_temp = status["extruder"]["target"];
                bool can_extrude = status["extruder"]["can_extrude"];
                printer.can_extrude = can_extrude == true;
            }

            if (status.containsKey("heater_bed"))
            {
                printer.bed_temp = status["heater_bed"]["temperature"];
                printer.bed_target_temp = status["heater_bed"]["target"];
            }

            if (status.containsKey("toolhead"))
            {
                const char *homed_axis = status["toolhead"]["homed_axes"];
                printer.homed_axis = strcmp(homed_axis, "xyz") == 0;
            }

            if (status.containsKey("gcode_move"))
            {
                printer.position[0] = status["gcode_move"]["gcode_position"][0];
                printer.position[1] = status["gcode_move"]["gcode_position"][1];
                printer.position[2] = status["gcode_move"]["gcode_position"][2];
                bool absolute_coords = status["gcode_move"]["absolute_coordinates"];
                printer.absolute_coords = absolute_coords == true;
            }

            if (status.containsKey("virtual_sdcard"))
            {
                printer.print_progress = status["virtual_sdcard"]["progress"];
            }

            if (status.containsKey("print_stats"))
            {
                const char *filename = status["print_stats"]["filename"];
                strcpy(filename_buff, filename);
                printer.print_filename = filename_buff;
                printer.elapsed_time_s = status["print_stats"]["print_duration"];
                printer.filament_used_mm = status["print_stats"]["filament_used"];

                const char *state = status["print_stats"]["state"];

                if (strcmp(state, "printing") == 0)
                {
                    printer_state = PRINTER_STATE_PRINTING;
                }
                else if (strcmp(state, "paused") == 0)
                {
                    printer_state = PRINTER_STATE_PAUSED;
                }
                else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
                {
                    printer_state = PRINTER_STATE_IDLE;
                }
            }

            // TODO: make a call to /server/files/metadata to get more accurate time estimates
            // https://moonraker.readthedocs.io/en/latest/web_api/#server-administration

            if (printer.state == PRINTER_STATE_PRINTING && printer.print_progress > 0)
            {
                printer.remaining_time_s = (printer.elapsed_time_s / printer.print_progress) - printer.elapsed_time_s;
            }

            lv_msg_send(DATA_PRINTER_DATA, &printer);
        }

        if (printer.state != printer_state || emit_state_update)
        {
            printer.state = printer_state;
            lv_msg_send(DATA_PRINTER_STATE, &printer);
        }
    }
    else
    {
        Serial.printf("Failed to fetch printer data: %d\n", httpCode);
    }
}

long last_data_update = 0;
const long data_update_interval = 1500;

void data_loop()
{
    if (millis() - last_data_update < data_update_interval)
        return;

    last_data_update = millis();

    fetch_printer_data();
}

void data_setup()
{
    printer.print_filename = filename_buff;
    fetch_printer_data();
    macros_query_setup();
}