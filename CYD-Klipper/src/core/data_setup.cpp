
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
#include "printer_integration.hpp"

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

    if (get_current_printer_data()->state == PrinterStateOffline)
    {
        if (!get_current_printer()->connect())
        {
            LOG_LN("Failed to connect to printer");
            unfreeze_request_thread();
            return;
        }
    }

    bool fetch_result = get_current_printer()->fetch();
    unfreeze_request_thread();

    freeze_render_thread();
    if (!fetch_result)
    {
        LOG_LN("Failed to fetch printer data")
        get_current_printer()->disconnect();
    }

    get_current_printer()->AnnouncePrinterData();
    unfreeze_render_thread();
}

void fetch_printer_data_minimal()
{
    // TODO
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
    printer_minimal = (PrinterMinimal*)calloc(sizeof(PrinterMinimal), PRINTER_CONFIG_COUNT);
    semaphore_init();
    printer.print_filename = filename_buff;
    fetch_printer_data();

    freeze_render_thread();
    xTaskCreatePinnedToCore(data_loop_background, "data_loop_background", 5000, NULL, 2, &background_loop, 0);
}
