
#include "data_setup.h"
#include "lvgl.h"
#include "../conf/global_config.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "macros_query.h"
#include <UrlEncode.h>
#include "http_client.h"
#include "../ui/ui_utils.h"
#include "macros_query.h"

Printer printer = {0};
PrinterMinimal *printer_minimal;
int klipper_request_consecutive_fail_count = 999;
char filename_buff[512] = {0};
SemaphoreHandle_t freezeRenderThreadSemaphore, freezeRequestThreadSemaphore;
const long data_update_interval = 780;
unsigned char lock_absolute_relative_mode_swap = 0;

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
    LOG_F(("Sending gcode: %s\n", gcode))

    SETUP_HTTP_CLIENT_FULL("/printer/gcode/script?script=" + urlEncode(gcode), false, wait ? 5000 : 750);
    try
    {
        client.GET();
    }
    catch (...)
    {
        LOG_LN("Failed to send gcode");
    }
}

void send_estop()
{
    LOG_LN("Sending estop");

    SETUP_HTTP_CLIENT_FULL("/printer/emergency_stop", false, 5000);
    try
    {
        client.GET();
    }
    catch (...)
    {
        LOG_LN("Failed to send estop");
    }
}

int get_slicer_time_estimate_s()
{
    if (printer.state == PRINTER_STATE_IDLE)
        return 0;
    
    delay(10);

    SETUP_HTTP_CLIENT("/server/files/metadata?filename=" + urlEncode(printer.print_filename));

    int httpCode = client.GET();

    if (httpCode != 200) 
        return 0;
    
    JsonDocument doc;
    deserializeJson(doc, client.getStream());
    int time_estimate_s = doc["result"]["estimated_time"];
    LOG_F(("Got slicer time estimate: %ds\n", time_estimate_s))
    return time_estimate_s;
}

void move_printer(const char* axis, float amount, bool relative) {
    if (!printer.homed_axis || printer.state == PRINTER_STATE_PRINTING)
        return;

    char gcode[64];
    const char* extra = (amount > 0) ? "+" : "";
    const char* start = "";
    const char* end = "";

    bool absolute_coords = printer.absolute_coords;

    if (absolute_coords && relative) {
        start = "G91\n";
    }
    else if (!absolute_coords && !relative) {
        start = "G90\n";
    }

    if (absolute_coords && relative) {
        end = "\nG90";
    }
    else if (!absolute_coords && !relative) {
        end = "\nG91";
    }

    sprintf(gcode, "%sG1 %s%s%.3f F6000%s", start, axis, extra, amount, end);
    send_gcode(true, gcode);

    lock_absolute_relative_mode_swap = 2;
}

int last_slicer_time_query = -15000;

void fetch_printer_data()
{
    freeze_request_thread();
    PRINTER_CONFIG *config = get_current_printer_config();
    SETUP_HTTP_CLIENT("/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan&display_status")

    int httpCode = client.GET();
    delay(10);
    if (httpCode == 200)
    {
        int printer_state = printer.state;

        if (printer.state == PRINTER_STATE_OFFLINE)
        {
            printer.state = PRINTER_STATE_ERROR;
        }

        klipper_request_consecutive_fail_count = 0;
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto status = doc["result"]["status"];
        bool emit_state_update = false;
        
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
            else if ((strcmp(state, "shutdown") == 0 || strcmp(state, "error") == 0) && printer.state != PRINTER_STATE_ERROR)
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

                if (lock_absolute_relative_mode_swap > 0)
                {
                    lock_absolute_relative_mode_swap--;
                }
                else 
                {
                    printer.absolute_coords = absolute_coords == true;
                }

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
                strcpy(filename_buff, filename == NULL ? "" : filename);
                printer.print_filename = filename_buff;
                printer.elapsed_time_s = status["print_stats"]["total_duration"];
                printer.printed_time_s = status["print_stats"]["print_duration"];
                printer.filament_used_mm = status["print_stats"]["filament_used"];
                printer.total_layers = status["print_stats"]["info"]["total_layer"];
                printer.current_layer = status["print_stats"]["info"]["current_layer"];

                const char *state = status["print_stats"]["state"];

                if (state == nullptr)
                {
                    // Continue
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

            if (status.containsKey("display_status"))
            {
                printer.print_progress = status["display_status"]["progress"];
                const char* message = status["display_status"]["message"];

                if (!global_config.disable_m117_messaging)
                {
                    lv_create_popup_message(message, 10000);
                }
            }

            if (printer.state == PRINTER_STATE_PRINTING && printer.print_progress > 0)
            {
                float remaining_time_s_percentage = (printer.printed_time_s / printer.print_progress) - printer.printed_time_s;
                float remaining_time_s_slicer = 0;

                if (printer.slicer_estimated_print_time_s > 0)
                {
                    remaining_time_s_slicer = printer.slicer_estimated_print_time_s - printer.printed_time_s;
                }

                if (remaining_time_s_slicer <= 0 || config->remaining_time_calc_mode == REMAINING_TIME_CALC_PERCENTAGE)
                {
                    printer.remaining_time_s = remaining_time_s_percentage;
                }
                else if (config->remaining_time_calc_mode == REMAINING_TIME_CALC_INTERPOLATED)
                {
                    printer.remaining_time_s = remaining_time_s_percentage * printer.print_progress + remaining_time_s_slicer * (1 - printer.print_progress);
                }
                else if (config->remaining_time_calc_mode == REMAINING_TIME_CALC_SLICER)
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
            printer.state = printer_state;
            lv_msg_send(DATA_PRINTER_STATE, &printer);
        }
        
        if (printer.state == PRINTER_STATE_PRINTING && millis() - last_slicer_time_query > 30000 && printer.slicer_estimated_print_time_s <= 0)
        {
            delay(10);
            last_slicer_time_query = millis();
            printer.slicer_estimated_print_time_s = get_slicer_time_estimate_s();
        }

        unfreeze_render_thread();
    }
    else
    {
        unfreeze_request_thread();
        klipper_request_consecutive_fail_count++;

        if (klipper_request_consecutive_fail_count == 5) 
        {
            freeze_render_thread();
            printer.state = PRINTER_STATE_OFFLINE;
            lv_msg_send(DATA_PRINTER_STATE, &printer);
            unfreeze_render_thread();
        }

        LOG_F(("Failed to fetch printer data: %d\n", httpCode))
    }
}

void fetch_printer_data_minimal()
{
    PrinterMinimal data[PRINTER_CONFIG_COUNT] = {0};

    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++){
        PRINTER_CONFIG *config = &global_config.printer_config[i];

        if (!config->ip_configured)
        {
            data[i].state = PRINTER_STATE_OFFLINE;
            continue;
        }

        delay(10);
        HTTPClient client;
        configure_http_client(client, get_full_url("/printer/objects/query?webhooks&print_stats&virtual_sdcard", config), true, 1000, config);
        freeze_request_thread();

        int httpCode = client.GET();
        delay(10);
        if (httpCode == 200)
        {
            if (data[i].state == PRINTER_STATE_OFFLINE)
            {
                data[i].state = PRINTER_STATE_ERROR;
            }

            data[i].power_devices = power_devices_count(config);
            JsonDocument doc;
            deserializeJson(doc, client.getStream());
            auto status = doc["result"]["status"];

            unfreeze_request_thread();

            if (status.containsKey("webhooks"))
            {
                const char *state = status["webhooks"]["state"];

                if (strcmp(state, "ready") == 0 && data[i].state == PRINTER_STATE_ERROR)
                {
                    data[i].state = PRINTER_STATE_IDLE;
                }
                else if (strcmp(state, "shutdown") == 0 && data[i].state != PRINTER_STATE_ERROR)
                {
                    data[i].state = PRINTER_STATE_ERROR;
                }
            }

            if (data[i].state != PRINTER_STATE_ERROR)
            {
                if (status.containsKey("virtual_sdcard"))
                {
                    data[i].print_progress = status["virtual_sdcard"]["progress"];
                }

                if (status.containsKey("print_stats"))
                {
                    const char *state = status["print_stats"]["state"];

                    if (state == nullptr)
                    {
                        data[i].state = PRINTER_STATE_ERROR;
                    }
                    else if (strcmp(state, "printing") == 0)
                    {
                        data[i].state = PRINTER_STATE_PRINTING;
                    }
                    else if (strcmp(state, "paused") == 0)
                    {
                        data[i].state = PRINTER_STATE_PAUSED;
                    }
                    else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
                    {
                        data[i].state = PRINTER_STATE_IDLE;
                    }
                }
            }
        }
        else 
        {
            data[i].state = PRINTER_STATE_OFFLINE;
            data[i].power_devices = power_devices_count(config);
            unfreeze_request_thread();
        }
    }

    freeze_render_thread();
    memcpy(printer_minimal, data, sizeof(PrinterMinimal) * PRINTER_CONFIG_COUNT);
    lv_msg_send(DATA_PRINTER_MINIMAL, NULL);
    unfreeze_render_thread();
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
    int loop_iter = 20;
    while (true){
        delay(data_update_interval);
        fetch_printer_data();
        if (global_config.multi_printer_mode) {
            if (loop_iter++ > 20){
                fetch_printer_data_minimal();
                loop_iter = 0;
            }
        }
    }
}

TaskHandle_t background_loop;

void data_setup()
{
    printer_minimal = (PrinterMinimal *)calloc(sizeof(PrinterMinimal), PRINTER_CONFIG_COUNT);
    semaphore_init();
    printer.print_filename = filename_buff;
    fetch_printer_data();

    freeze_render_thread();
    xTaskCreatePinnedToCore(data_loop_background, "data_loop_background", 5000, NULL, 2, &background_loop, 0);
}
