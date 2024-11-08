#include "serial_klipper_printer_integration.hpp"
#include <HardwareSerial.h>
#include <UrlEncode.h>

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
int make_serial_request(JsonDocument &out, int timeout_ms, HttpRequestType requestType, const char* endpoint)
{
    clear_serial_buffer();
    // TODO: Add semaphore here
    if (!Serial.availableForWrite())
    {
        return -1;
    }

    char buff[10];
    sprintf(buff, "%d ", timeout_ms);

    // TODO: Maybe use printf?
    Serial.write(buff);
    Serial.write(requestType == HttpGet ? "GET" : "POST");
    Serial.write(' ');
    Serial.write(endpoint);
    Serial.write('\n');

    if (timeout_ms <= 0)
    {
        return 200;
    }
    unsigned long _m = millis();
    while (!Serial.available() && millis() < _m + timeout_ms + 10) delay(1);

    if (!Serial.available())
    {
        return -2;
    }

    Serial.readBytes(buff, 4);
    buff[3] = 0;

    if (buff[0] < '0' || buff[0] > '9')
    {
        clear_serial_buffer();
        return -3;
    }

    int status_code = atoi(buff);

    if (status_code < 200 || status_code >= 300)
    {
        clear_serial_buffer();
        return -4;
    }

    auto result = deserializeJson(out, Serial);
    return result == DeserializationError::Ok;
}

bool make_serial_request_nocontent(HttpRequestType requestType, const char* endpoint)
{
    JsonDocument doc;
    make_serial_request(doc, 0, requestType, endpoint);
    return true;
}

bool SerialKlipperPrinter::connect()
{
    return connection_test_serial_klipper(printer_config) == KlipperConnectionStatus::ConnectOk;
}

bool SerialKlipperPrinter::fetch()
{
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan&display_status") == 200)
    {
        if (printer_data.state == PrinterStateOffline)
        {
            printer_data.state = PrinterStateError;
        }

        klipper_request_consecutive_fail_count = 0;
        parse_state(doc);
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

    if (make_serial_request(doc, 1000, HttpGet, "/printer/objects/query?webhooks&print_stats&virtual_sdcard") == 200)
    {
        data.state = PrinterState::PrinterStateIdle;
        parse_state_min(doc, &data);
        doc.clear();
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
    if (make_serial_request(doc, 1000, HttpGet, "/printer/gcode/help") == 200)
    {
        return parse_macros(doc);
    }

    return macros;
}

int SerialKlipperPrinter::get_macros_count()
{
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/printer/gcode/help") == 200)
    {
        return parse_macros_count(doc);
    }

    return 0; 
}

PowerDevices SerialKlipperPrinter::get_power_devices()
{
    PowerDevices power_devices = {0};
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/machine/device_power/devices") == 200)
    {
        return parse_power_devices(doc);
    }

    return power_devices;   
}

int SerialKlipperPrinter::get_power_devices_count()
{
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/machine/device_power/devices") == 200)
    {
        return parse_power_devices_count(doc);
    }

    return 0;     
}

bool SerialKlipperPrinter::set_power_device_state(const char* device_name, bool state)
{
    JsonDocument doc;
    String request = "/machine/device_power/device?device=" + urlEncode(device_name) + "&action=" + (state ? "on" : "off");
    return make_serial_request(doc, 1000, HttpPost, request.c_str());
}

Files SerialKlipperPrinter::get_files()
{
    // TODO: Stubbed
    Files files = {0};
    files.success = false;
    return files;
}

bool SerialKlipperPrinter::start_file(const char* filename)
{
    JsonDocument doc;
    String request = "/printer/print/start?filename=" + urlEncode(filename);
    return make_serial_request(doc, 1000, HttpPost, request.c_str());
}

Thumbnail SerialKlipperPrinter::get_32_32_png_image_thumbnail(const char* gcode_filename)
{
    // TODO: Stubbed
    Thumbnail thumbnail = {0};
    thumbnail.success = false;
    return thumbnail;
}

bool SerialKlipperPrinter::send_gcode(const char* gcode, bool wait)
{
    JsonDocument doc;
    String request = "/printer/gcode/script?script=" + urlEncode(gcode);
    return wait
        ? make_serial_request(doc, 5000, HttpGet, request.c_str())
        : make_serial_request_nocontent(HttpGet, request.c_str());
}

KlipperConnectionStatus connection_test_serial_klipper(PrinterConfiguration* config)
{
    JsonDocument doc;
    if (make_serial_request(doc, 1000, HttpGet, "/printer/info") != 200)
    {
        return KlipperConnectionStatus::ConnectOk;
    }

    return KlipperConnectionStatus::ConnectFail;
}