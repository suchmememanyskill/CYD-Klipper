
#include "data_setup.h"
#include <esp_task_wdt.h>
#include <UrlEncode.h>
#include "printer_integration.hpp"
#include "klipper/klipper_printer_integration.hpp"
#include "klipper-serial/serial_klipper_printer_integration.hpp"
#include "bambu/bambu_printer_integration.hpp"

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
    PrinterDataMinimal data[PRINTER_CONFIG_COUNT] = {{}};
    for (int i = 0; i < get_printer_count(); i++)
    {
        freeze_request_thread();
        BasePrinter* printer = get_printer(i);
        unfreeze_request_thread();
        data[i] = printer->fetch_min();
    }
    freeze_render_thread();
    announce_printer_data_minimal(data);
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
    BasePrinter** available_printers = (BasePrinter**)malloc(sizeof(BasePrinter*) * PRINTER_CONFIG_COUNT);
    int count = 0;
    int true_current_printer_index = 0;
    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++)
    {
        if (global_config.printer_config[i].setup_complete)
        {
            if (global_config.printer_index == i)
            {
                true_current_printer_index = count;;
            }

            switch (global_config.printer_config[i].printer_type)
            {
                case PrinterType::PrinterTypeKlipper:
                    available_printers[count++] = new KlipperPrinter(i);
                    break;
                case PrinterType::PrinterTypeBambuLocal:
                    available_printers[count++] = new BambuPrinter(i);
                    break;
                case PrinterType::PrinterTypeKlipperSerial:
                    available_printers[count++] = new SerialKlipperPrinter(i);
                    break;
            }
        }
    }
    
    initialize_printers(available_printers, count);
    set_current_printer(true_current_printer_index);
    LOG_F(("Free heap after printer creation: %d bytes\n", esp_get_free_heap_size()));
    semaphore_init();
    fetch_printer_data();
    freeze_render_thread();
    xTaskCreatePinnedToCore(data_loop_background, "data_loop_background", 5000, NULL, 2, &background_loop, 0);
}
