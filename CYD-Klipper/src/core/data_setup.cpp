
#include "data_setup.h"
#include "lvgl.h"
#include "../conf/global_config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "macros_query.h"
#include <UrlEncode.h>

const char *printer_state_messages[] = {
    "Error",
    "Idle",
    "Printing"};

Printer printer = {0};
int klipper_request_consecutive_fail_count = 0;
char filename_buff[512] = {0};
SemaphoreHandle_t freezeRenderThreadSemaphore, freezeRequestThreadSemaphore;
const long data_update_interval = 780;

void semaphore_init(){
    freezeRenderThreadSemaphore = xSemaphoreCreateMutex();
    freezeRequestThreadSemaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(freezeRenderThreadSemaphore);
    xSemaphoreGive(freezeRequestThreadSemaphore);
}

void freeze_request_thread(){
    xSemaphoreTake(freezeRequestThreadSemaphore, portMAX_DELAY);
}

void unfreeze_request_thread(){
    xSemaphoreGive(freezeRequestThreadSemaphore);
}

void freeze_render_thread(){
    xSemaphoreTake(freezeRenderThreadSemaphore, portMAX_DELAY);
}

void unfreeze_render_thread(){
    xSemaphoreGive(freezeRenderThreadSemaphore);
}

void send_gcode(bool wait, const char *gcode)
{
    Serial.printf("Sending gcode: %s\n", gcode);
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/printer/gcode/script?script=%s", global_config.klipperHost, global_config.klipperPort, urlEncode(gcode).c_str());
    HTTPClient client;
    client.begin(buff);

    if (global_config.auth_configured)
        client.addHeader("X-Api-Key", global_config.klipper_auth);

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

int get_slicer_time_estimate_s()
{
    if (printer.state == PRINTER_STATE_IDLE)
        return 0;
    
    delay(10);

    char buff[256] = {};
    sprintf(buff, "http://%s:%d/server/files/metadata?filename=%s", global_config.klipperHost, global_config.klipperPort, urlEncode(printer.print_filename).c_str());
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(buff);

    if (global_config.auth_configured)
        client.addHeader("X-Api-Key", global_config.klipper_auth);

    int httpCode = client.GET();

    if (httpCode != 200) 
        return 0;
    
    JsonDocument doc;
    deserializeJson(doc, client.getStream());
    int time_estimate_s = doc["result"]["estimated_time"];
    Serial.printf("Got slicer time estimate: %ds\n", time_estimate_s);
    return time_estimate_s;
}

void move_printer(const char* axis, float amount, bool relative) {
    if (!printer.homed_axis || printer.state == PRINTER_STATE_PRINTING)
        return;

    char gcode[64];
    const char* extra = (amount > 0) ? "+" : "";

    bool absolute_coords = printer.absolute_coords;

    if (absolute_coords && relative) {
        send_gcode(true, "G91");
    }
    else if (!absolute_coords && !relative) {
        send_gcode(true, "G90");
    }

    sprintf(gcode, "G1 %s%s%.3f F6000", axis, extra, amount);
    send_gcode(true, gcode);

    if (absolute_coords && relative) {
        send_gcode(true, "G90");
    }
    else if (!absolute_coords && !relative) {
        send_gcode(true, "G91");
    }
}

void fetch_printer_data()
{
    freeze_request_thread();
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan", global_config.klipperHost, global_config.klipperPort);
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(buff);

    if (global_config.auth_configured)
        client.addHeader("X-Api-Key", global_config.klipper_auth);

    int httpCode = client.GET();
    delay(10);
    if (httpCode == 200)
    {
        klipper_request_consecutive_fail_count = 0;
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto status = doc["result"]["status"];
        bool emit_state_update = false;
        int printer_state = printer.state;
        delay(10);
        unfreeze_request_thread();
        freeze_render_thread();

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
                printer.pressure_advance = status["extruder"]["pressure_advance"];
                printer.smooth_time = status["extruder"]["smooth_time"];
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
                printer.gcode_offset[0] = status["gcode_move"]["homing_origin"][0];
                printer.gcode_offset[1] = status["gcode_move"]["homing_origin"][1];
                printer.gcode_offset[2] = status["gcode_move"]["homing_origin"][2];
                bool absolute_coords = status["gcode_move"]["absolute_coordinates"];
                printer.absolute_coords = absolute_coords == true;
                printer.speed_mult = status["gcode_move"]["speed_factor"];
                printer.extrude_mult = status["gcode_move"]["extrude_factor"];
                printer.feedrate_mm_per_s = status["gcode_move"]["speed"];
                printer.feedrate_mm_per_s /= 60; // convert mm/m to mm/s
            }

            if (status.containsKey("fan"))
            {
                printer.fan_speed = status["fan"]["speed"];
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
                printer.total_layers = status["print_stats"]["info"]["total_layer"];
                printer.current_layer = status["print_stats"]["info"]["current_layer"];

                const char *state = status["print_stats"]["state"];

                if (state == nullptr)
                {
                    printer_state = PRINTER_STATE_ERROR;
                }
                else if (strcmp(state, "printing") == 0)
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
                float remaining_time_s_percentage = (printer.elapsed_time_s / printer.print_progress) - printer.elapsed_time_s;
                float remaining_time_s_slicer = 0;

                if (printer.slicer_estimated_print_time_s > 0)
                {
                    remaining_time_s_slicer = printer.slicer_estimated_print_time_s - printer.elapsed_time_s;
                }

                if (remaining_time_s_slicer <= 0 || global_config.remaining_time_calc_mode == REMAINING_TIME_CALC_PERCENTAGE)
                {
                    printer.remaining_time_s = remaining_time_s_percentage;
                }
                else if (global_config.remaining_time_calc_mode == REMAINING_TIME_CALC_INTERPOLATED)
                {
                    printer.remaining_time_s = remaining_time_s_percentage * printer.print_progress + remaining_time_s_slicer * (1 - printer.print_progress);
                }
                else if (global_config.remaining_time_calc_mode == REMAINING_TIME_CALC_SLICER)
                {
                    printer.remaining_time_s = remaining_time_s_slicer;
                }
            }

            if (printer.remaining_time_s < 0)
            {
                printer.remaining_time_s = 0;
            }

            if (printer.state == PRINTER_STATE_IDLE)
            {
                printer.slicer_estimated_print_time_s = 0;
            }

            lv_msg_send(DATA_PRINTER_DATA, &printer);
        }

        if (printer.state != printer_state || emit_state_update)
        {
            if (printer_state == PRINTER_STATE_PRINTING){
                printer.slicer_estimated_print_time_s = get_slicer_time_estimate_s();
            }

            printer.state = printer_state;
            lv_msg_send(DATA_PRINTER_STATE, &printer);
        }

        unfreeze_render_thread();
    }
    else
    {
        klipper_request_consecutive_fail_count++;
        Serial.printf("Failed to fetch printer data: %d\n", httpCode);
        unfreeze_request_thread();
    }
}

void data_loop()
{
    // Causes other threads that are trying to lock the thread to actually lock it
    unfreeze_render_thread();
    delay(1);
    freeze_render_thread();
}

void data_loop_background(void * param){
    esp_task_wdt_init(10, true);
    while (true){
        delay(data_update_interval);
        fetch_printer_data();
    }
}

TaskHandle_t background_loop;

void data_setup()
{
    semaphore_init();
    printer.print_filename = filename_buff;
    fetch_printer_data();
    macros_query_setup();
    freeze_render_thread();
    xTaskCreatePinnedToCore(data_loop_background, "data_loop_background", 5000, NULL, 0, &background_loop, 0);
}
