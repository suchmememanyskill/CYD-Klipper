#include "octoprint_printer_integration.hpp"
#include "../../conf/global_config.h"
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ArduinoJson.h>
#include <list>

const char* COMMAND_CONNECT = "{\"command\":\"connect\"}";
const char* COMMAND_DISCONNECT = "{\"command\":\"disconnect\"}";
const char* COMMAND_HOME = "{\"command\":\"home\",\"axes\":[\"x\",\"y\",\"z\"]}";

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

bool OctoPrinter::get_request(const char* endpoint, int timeout_ms, bool stream)
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

bool OctoPrinter::post_request(const char* endpoint, const char* body, int timeout_ms, bool stream)
{
    HTTPClient client;

    if (timeout_ms <= 0)
    {
        timeout_ms = 500;
    }

    configure_http_client(client, endpoint, stream, timeout_ms, printer_config);

    if (body[0] == '{' || body[0] == '[')
    {
        client.addHeader("Content-Type", "application/json");
    }

    int result = client.POST(body);
    return result >= 200 && result < 300;
}

// TODO: Make array in the first place, don't split on \n
bool OctoPrinter::send_gcode(const char* gcode, bool wait)
{
    JsonDocument doc;
    JsonArray array = doc["commands"].to<JsonArray>();
    const char* last_line_start = gcode;
    char out_buff[512];

    for (const char* iter = gcode;; iter++)
    {
        if (*iter == '\n' || *iter == '\0')
        {
            const char *prev_char = iter - 1;

            if (iter == last_line_start)
            {
                continue;
            }

            char* buff = (char*)malloc(sizeof(char) * (prev_char - gcode) + 1);
            buff[prev_char - gcode] = '\0';
            memcpy(buff, last_line_start, prev_char - gcode);
            last_line_start = iter + 1;
            array.add(last_line_start);
        }

        if (*iter == '\0')
        {
            break;
        }
    }

    if (serializeJson(doc, out_buff, 512) != 512)
    {
        return false;
    }

    return post_request("/api/printer/command/custom", out_buff);
}

bool OctoPrinter::move_printer(const char* axis, float amount, bool relative)
{
    JsonDocument doc;
    char out_buff[512];

    doc["command"] = "jog";
    doc[axis] = amount;
    doc["absolute"] = !relative;

    if (serializeJson(doc, out_buff, 512) != 512)
    {
        return false;
    }

    return post_request("/api/printer/printhead", out_buff);
}

bool OctoPrinter::execute_feature(PrinterFeatures feature)
{
    switch (feature)
    {
        case PrinterFeatureRetryError:
            if (no_printer)
            {
                bool a = post_request("/api/connection", COMMAND_CONNECT);
                LOG_F(("Retry error: %d\n", a));
                return a;
            }
        case PrinterFeatureHome:
            return post_request("/api/printer/printhead", COMMAND_HOME);
        case PrinterFeatureDisableSteppers:
            return send_gcode("M18");
        default:
            LOG_F(("Unsupported printer feature %d", feature));
            break;
    }

    return false;
}

bool OctoPrinter::connect()
{
    return connection_test_octoprint(printer_config) == OctoConnectionStatus::OctoConnectOk;
}

bool OctoPrinter::fetch()
{
    HTTPClient client;
    HTTPClient client2;
    configure_http_client(client, "/api/printer", true, 1000, printer_config);

    int http_code = client.GET();

    if (http_code == 200)
    {
        no_printer = false;
        request_consecutive_fail_count = 0;
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        parse_printer_state(doc);

        doc.clear();
        configure_http_client(client2, "/api/job", true, 1000, printer_config);
        if (client2.GET() == 200)
        {
            deserializeJson(doc, client2.getStream());
            parse_job_state(doc);
        }
        else
        {
            printer_data.state = PrinterStateOffline;
            return false;
        }
    }
    else if (http_code == 409)
    {
        no_printer = true;
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        parse_error(doc);
    }
    else 
    {
        request_consecutive_fail_count++;
        LOG_LN("Failed to fetch printer data");

        if (request_consecutive_fail_count >= 5) 
        {
            printer_data.state = PrinterStateOffline;
            return false;
        }
    }

    return true;
}

PrinterDataMinimal OctoPrinter::fetch_min()
{
    return {};
}

void OctoPrinter::disconnect()
{

}

const char* MACRO_AUTOLEVEL = "Auto-Level (G28+G29)";
const char* MACRO_DISCONNECT = "Disconnect printer";

Macros OctoPrinter::get_macros()
{
    if (printer_data.state == PrinterStatePrinting || printer_data.state == PrinterStateOffline)
    {
        Macros macros = {0};
        macros.success = false;
        return macros;
    }

    Macros macros = {0};
    macros.count = 2;
    macros.macros = (char **)malloc(sizeof(char *) * macros.count);
    macros.macros[0] = (char *)malloc(strlen(MACRO_AUTOLEVEL) + 1);
    strcpy(macros.macros[0], MACRO_AUTOLEVEL);
    macros.macros[1] = (char *)malloc(strlen(MACRO_DISCONNECT) + 1);
    strcpy(macros.macros[1], MACRO_DISCONNECT);
    macros.success = true;
    return macros;
}

int OctoPrinter::get_macros_count()
{
    return (printer_data.state == PrinterStatePrinting || printer_data.state == PrinterStateOffline) ? 0 : 2;
}

bool OctoPrinter::execute_macro(const char* macro)
{
    if (strcmp(macro, MACRO_AUTOLEVEL) == 0)
    {
        return send_gcode("G28\nG29");
    }
    else if (strcmp(macro, MACRO_DISCONNECT) == 0)
    {
        if (printer_data.state == PrinterStatePrinting || printer_data.state == PrinterStateOffline)
        {
            return false;
        }
        
        return post_request("/api/connection", COMMAND_DISCONNECT);
    }

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