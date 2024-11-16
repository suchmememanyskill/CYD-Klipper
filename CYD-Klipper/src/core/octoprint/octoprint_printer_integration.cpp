#include "octoprint_printer_integration.hpp"
#include "../../conf/global_config.h"
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ArduinoJson.h>
#include <list>

void configure_http_client(HTTPClient &client, String url_part, bool stream, int timeout, PrinterConfiguration* printer_config)
{
    client.useHTTP10(stream);

    if (timeout > 0){
        client.setTimeout(timeout);
        client.setConnectTimeout(timeout);
    }

    client.begin("http://" + String(printer_config->klipper_host) + ":" + String(printer_config->klipper_port) + url_part);

    if (printer_config->auth_configured) {
        client.addHeader("X-Api-Key", printer_config->klipper_auth);
    }
}

bool OctoPrinter::make_request(const char* endpoint, HttpRequestType requestType, int timeout_ms, bool stream)
{
    HTTPClient client;

    if (timeout_ms <= 0)
    {
        timeout_ms = 500;
    }

    configure_http_client(client, endpoint, stream, timeout_ms, printer_config);
    int result = client.GET();
    return result >= 200 && result < 300;
}

bool OctoPrinter::make_request(JsonDocument& doc, const char* endpoint, HttpRequestType requestType, int timeout_ms, bool stream)
{
    HTTPClient client;

    if (timeout_ms <= 0)
    {
        timeout_ms = 500;
    }

    configure_http_client(client, endpoint, stream, timeout_ms, printer_config);
    int result = client.GET();

    if (result >= 200 && result < 300)
    {
        auto result = deserializeJson(doc, client.getStream());
        return result == DeserializationError::Ok;
    }

    return false;
}

bool OctoPrinter::move_printer(const char* axis, float amount, bool relative)
{
    return false;
}

bool OctoPrinter::execute_feature(PrinterFeatures feature)
{
    return false;
}

bool OctoPrinter::connect()
{
    return false;
}

bool OctoPrinter::fetch()
{
    return false;
}

PrinterDataMinimal OctoPrinter::fetch_min()
{
    return {};
}

void OctoPrinter::disconnect()
{

}

Macros OctoPrinter::get_macros()
{
    return {};
}

int OctoPrinter::get_macros_count()
{
    return 0;
}

bool OctoPrinter::execute_macro(const char* macro)
{
    return false;
}

PowerDevices OctoPrinter::get_power_devices()
{
    return {};
}

int OctoPrinter::get_power_devices_count()
{
    return 0;
}

bool OctoPrinter::set_power_device_state(const char* device_name, bool state)
{
    return false;
}

Files OctoPrinter::get_files()
{
    return {};
}

bool OctoPrinter::start_file(const char* filename)
{
    return false;
}

Thumbnail OctoPrinter::get_32_32_png_image_thumbnail(const char* gcode_filename)
{
    return {};
}

bool OctoPrinter::set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature)
{
    return false;
}

OctoConnectionStatus connection_test_octoprint(PrinterConfiguration* config)
{
    HTTPClient client;
    configure_http_client(client, "/api/version", false, 1000, config);

    int http_code = client.GET();
    if (http_code == 200)
    {
        return OctoConnectionStatus::OctoConnectOk;
    }
    else if (http_code == 401 || http_code == 403)
    {
        return OctoConnectionStatus::OctoConnectKeyFail;
    }
    else
    {
        return OctoConnectionStatus::OctoConnectFail;
    }
}