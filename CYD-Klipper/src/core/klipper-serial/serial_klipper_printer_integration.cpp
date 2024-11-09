#include "serial_klipper_printer_integration.hpp"
#include <HardwareSerial.h>
#include <UrlEncode.h>
#include "../../ui/serial/serial_console.h"

bool is_semaphores_initialized = false;
SemaphoreHandle_t freeze_serial_thread_semaphore;

void semaphore_init_serial()
{
    if (is_semaphores_initialized)
    {
        return;
    }

    freeze_serial_thread_semaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(freeze_serial_thread_semaphore);
    is_semaphores_initialized = true;
}

void freeze_serial_thread()
{
    xSemaphoreTake(freeze_serial_thread_semaphore, portMAX_DELAY);
}

void unfreeze_serial_thread()
{
    xSemaphoreGive(freeze_serial_thread_semaphore);
}

enum HttpRequestType
{
    HttpPost,
    HttpGet
};

void clear_serial_buffer()
{
    while (Serial.available())
    {
        Serial.read();
    };
}

// Request: {timeout} {method} {endpoint}
// Response: {status code} {body}
bool make_serial_request(JsonDocument &out, int timeout_ms, HttpRequestType requestType, const char* endpoint)
{
    semaphore_init_serial();
    freeze_serial_thread();
    serial_console::global_disable_serial_console = true;
    temporary_config.debug = false;
    char buff[10];
    clear_serial_buffer();

    // TODO: Add semaphore here
    if (!Serial.availableForWrite())
    {
        unfreeze_serial_thread();
        return false;
    }

    Serial.printf("HTTP_REQUEST %d %s %s\n", timeout_ms, requestType == HttpGet ? "GET" : "POST", endpoint);

    if (timeout_ms <= 0)
    {
        unfreeze_serial_thread();
        return true;
    }
    unsigned long _m = millis();
    while (!Serial.available() && millis() < _m + timeout_ms + 10) delay(1);

    if (!Serial.available())
    {
        Serial.println("Timeout...");
        unfreeze_serial_thread();
        return false;
    }

    Serial.readBytes(buff, 4);
    buff[3] = 0;

    if (buff[0] < '0' || buff[0] > '9')
    {
        Serial.println("Invalid error code");
        clear_serial_buffer();
        unfreeze_serial_thread();
        return false;
    }

    int status_code = atoi(buff);

    if (status_code < 200 || status_code >= 300)
    {
        Serial.println("Non-200 error code");
        clear_serial_buffer();
        unfreeze_serial_thread();
        return false;
    }

    auto result = deserializeJson(out, Serial);
    Serial.printf("Deserialization result: %s\n", result.c_str());
    bool success = result == DeserializationError::Ok;

    if (!success)
    {
        unfreeze_serial_thread();
    }

    return success;
}

typedef struct
{
    int len;
    unsigned char* data;
} BinaryResponse;

bool make_binary_request(BinaryResponse* data, int timeout_ms, HttpRequestType requestType, const char* endpoint)
{
    semaphore_init_serial();
    freeze_serial_thread();
    serial_console::global_disable_serial_console = true;
    temporary_config.debug = false;
    char buff[10];
    clear_serial_buffer();

    // TODO: Add semaphore here
    if (!Serial.availableForWrite() || timeout_ms <= 0)
    {
        unfreeze_serial_thread();
        return false;
    }

    Serial.printf("HTTP_BINARY %d %s %s\n", timeout_ms, requestType == HttpGet ? "GET" : "POST", endpoint);

    unsigned long _m = millis();
    while (!Serial.available() && millis() < _m + timeout_ms + 10) delay(1);

    if (!Serial.available())
    {
        Serial.println("Timeout...");
        unfreeze_serial_thread();
        return false;
    }

    Serial.readBytes(buff, 8);
    buff[9] = 0;

    if (buff[0] < '0' || buff[0] > '9')
    {
        Serial.println("Invalid length");
        clear_serial_buffer();
        unfreeze_serial_thread();
        return false;
    }

    int data_length = atoi(buff);

    if (data_length <= 0)
    {
        Serial.println("0 Length");
        clear_serial_buffer();
        unfreeze_serial_thread();
        return false;
    }

    data->len = data_length;
    data->data = (unsigned char*)malloc(data_length);

    if (data->data == NULL)
    {
        Serial.println("Failed to allocate memory");
        clear_serial_buffer();
        unfreeze_serial_thread();
        return false;
    }

    bool result = Serial.readBytes((char*)data->data, data_length) == data_length;
    unfreeze_serial_thread();

    if (!result)
    {
        free(data->data);
    }

    return result;
}

bool make_serial_request_nocontent(HttpRequestType requestType, const char* endpoint)
{
    JsonDocument doc;
    make_serial_request(doc, 0, requestType, endpoint);
    unfreeze_serial_thread();
    return true;
}

bool SerialKlipperPrinter::connect()
{
    return connection_test_serial_klipper(printer_config) == KlipperConnectionStatus::ConnectOk;
}

bool SerialKlipperPrinter::fetch()
{
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan&display_status"))
    {
        if (printer_data.state == PrinterStateOffline)
        {
            printer_data.state = PrinterStateError;
        }

        klipper_request_consecutive_fail_count = 0;
        parse_state(doc);
        unfreeze_serial_thread();
    }
    else
    {
        klipper_request_consecutive_fail_count++;
        if (klipper_request_consecutive_fail_count >= 5) 
        {
            printer_data.state = PrinterStateOffline;
            return false;
        }
    }

    return true;
}

PrinterDataMinimal SerialKlipperPrinter::fetch_min()
{
    JsonDocument doc;
    PrinterDataMinimal data = {};
    data.success = false;

    if (!printer_config->setup_complete)
    {
        data.state = PrinterStateOffline;
        return data;
    }

    data.success = true;

    if (make_serial_request(doc, 1000, HttpGet, "/printer/objects/query?webhooks&print_stats&virtual_sdcard"))
    {
        data.state = PrinterState::PrinterStateIdle;
        parse_state_min(doc, &data);
        unfreeze_serial_thread();
        data.power_devices = get_power_devices_count();
    }
    else 
    {
        data.state = PrinterState::PrinterStateOffline;
        data.power_devices = get_power_devices_count();       
    }

    return data;
}

Macros SerialKlipperPrinter::get_macros()
{
    Macros macros = {0};
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/printer/gcode/help"))
    {
        macros = parse_macros(doc);
        unfreeze_serial_thread();
    }

    return macros;
}

int SerialKlipperPrinter::get_macros_count()
{
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/printer/gcode/help"))
    {
        int count = parse_macros_count(doc);
        unfreeze_serial_thread();
        return count;
    }

    return 0; 
}

PowerDevices SerialKlipperPrinter::get_power_devices()
{
    PowerDevices power_devices = {0};
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/machine/device_power/devices"))
    {
        power_devices = parse_power_devices(doc);
        unfreeze_serial_thread();
    }

    return power_devices;   
}

int SerialKlipperPrinter::get_power_devices_count()
{
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/machine/device_power/devices"))
    {
        int count = parse_power_devices_count(doc);
        unfreeze_serial_thread();
        return count;
    }

    return 0;     
}

bool SerialKlipperPrinter::set_power_device_state(const char* device_name, bool state)
{
    String request = "/machine/device_power/device?device=" + urlEncode(device_name) + "&action=" + (state ? "on" : "off");
    return make_serial_request_nocontent(HttpGet, request.c_str());
}

Files SerialKlipperPrinter::get_files()
{
    Files files_result = {0};
    files_result.success = false;
    JsonDocument doc;
    LOG_F(("Heap space pre-file-parse: %d bytes\n", esp_get_free_heap_size()));
    std::list<FileSystemFile> files;

    auto timer_request = millis();
    bool result = make_serial_request(doc, 5000, HttpGet, "/server/files/list");
    auto timer_parse = millis();

    if (!result)
    {
        return files_result;
    }

    parse_file_list(doc, files, 20);
    unfreeze_serial_thread();

    files_result.available_files = (char**)malloc(sizeof(char*) * files.size());

    if (files_result.available_files == NULL){
        LOG_LN("Failed to allocate memory");

        for (auto file : files){
            free(file.name);
        }

        return files_result;
    }

    for (auto file : files){
        files_result.available_files[files_result.count++] = file.name;
    }

    files_result.success = true;

    LOG_F(("Heap space post-file-parse: %d bytes\n", esp_get_free_heap_size()))
    LOG_F(("Got %d files. Request took %dms, parsing took %dms\n", files.size(), timer_parse - timer_request, millis() - timer_parse))
    return files_result;
}

bool SerialKlipperPrinter::start_file(const char* filename)
{
    JsonDocument doc;
    String request = "/printer/print/start?filename=" + urlEncode(filename);
    return make_serial_request_nocontent(HttpPost, request.c_str());;
}

Thumbnail SerialKlipperPrinter::get_32_32_png_image_thumbnail(const char* gcode_filename)
{
    Thumbnail thumbnail = {0};
    JsonDocument doc;
    char* img_filename_path = NULL;

    String request = "/server/files/thumbnails?filename=" + urlEncode(gcode_filename);
    if (make_serial_request(doc, 1000, HttpGet, request.c_str()))
    {
        img_filename_path = parse_thumbnails(doc);
        unfreeze_serial_thread();
        doc.clear();
    }

    if (img_filename_path == NULL)
    {
        return thumbnail;
    }

    request = "/server/files/gcodes/" + urlEncode(img_filename_path);
    BinaryResponse data = {0};
    if (make_binary_request(&data, 2000, HttpGet, request.c_str()))
    {
        thumbnail.png = data.data;
        thumbnail.size = data.len;
        thumbnail.success = true;
    }

    free(img_filename_path);
    return thumbnail;
}

bool SerialKlipperPrinter::send_gcode(const char* gcode, bool wait)
{
    JsonDocument doc;
    String request = "/printer/gcode/script?script=" + urlEncode(gcode);

    if (!wait)
    {
        return make_serial_request_nocontent(HttpGet, request.c_str());
    }

    bool result = make_serial_request(doc, 5000, HttpGet, request.c_str());
    unfreeze_serial_thread();
    return result;
}

bool SerialKlipperPrinter::send_emergency_stop()
{
    return make_serial_request_nocontent(HttpGet, "/printer/emergency_stop");
}

int SerialKlipperPrinter::get_slicer_time_estimate_s()
{
    if (printer_data.state != PrinterStatePrinting && printer_data.state != PrinterStatePaused)
        return 0;

    String request = "/server/files/metadata?filename=" + urlEncode(printer_data.print_filename);
    JsonDocument doc;

    if (!make_serial_request(doc, 2000, HttpGet, request.c_str()))
    {
        return 0;
    }

    int estimate = parse_slicer_time_estimate(doc);
    unfreeze_serial_thread();

    return estimate;
}

KlipperConnectionStatus connection_test_serial_klipper(PrinterConfiguration* config)
{
    serial_console::global_disable_serial_console = true;
    temporary_config.debug = false;
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/printer/info"))
    {
        unfreeze_serial_thread();
        return KlipperConnectionStatus::ConnectOk;
    }

    return KlipperConnectionStatus::ConnectFail;
}