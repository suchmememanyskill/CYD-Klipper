#include "klipper_printer_integration.hpp"
#include "../../conf/global_config.h"
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ArduinoJson.h>
#include <list>

void KlipperPrinter::configure_http_client(HTTPClient &client, String url_part, bool stream, int timeout)
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

int KlipperPrinter::get_slicer_time_estimate_s()
{
    if (printer_data.state != PrinterStatePrinting && printer_data.state != PrinterStatePaused)
        return 0;

    HTTPClient client;
    configure_http_client(client, "/server/files/metadata?filename=" + urlEncode(printer_data.print_filename), true, 5000);
    int http_code = client.GET();

    if (http_code != 200) 
        return 0;
    
    JsonDocument doc;
    deserializeJson(doc, client.getStream());
    return parse_slicer_time_estimate(doc);
}

bool KlipperPrinter::send_gcode(const char *gcode, bool wait)
{
    HTTPClient client;
    configure_http_client(client, "/printer/gcode/script?script=" + urlEncode(gcode), false, wait ? 5000 : 750);
    LOG_F(("Sending gcode: %s\n", gcode))

    try
    {
        client.GET();
        return true;
    }
    catch (...)
    {
        LOG_LN("Failed to send gcode");
        return false;
    }
}

bool KlipperPrinter::move_printer(const char* axis, float amount, bool relative)
{
    if (!printer_data.homed_axis || printer_data.state == PrinterStatePrinting)
        return false;

    char gcode[64];
    const char* extra = (amount > 0) ? "+" : "";
    const char* start = "";
    const char* end = "";

    if (printer_data.absolute_coords && relative) {
        start = "G91\n";
    }
    else if (!printer_data.absolute_coords && !relative) {
        start = "G90\n";
    }

    if (printer_data.absolute_coords && relative) {
        end = "\nG90";
    }
    else if (!printer_data.absolute_coords && !relative) {
        end = "\nG91";
    }

    sprintf(gcode, "%sG1 %s%s%.3f F6000%s", start, axis, extra, amount, end);
    send_gcode(gcode);

    lock_absolute_relative_mode_swap = 2;
    return true;
}

bool KlipperPrinter::send_emergency_stop()
{
    HTTPClient client;
    configure_http_client(client, "/printer/emergency_stop", false, 5000);

    try
    {
        return client.GET() == 200;
    }
    catch (...)
    {
        LOG_LN("Failed to send estop");
        return false;
    }
}

bool KlipperPrinter::execute_feature(PrinterFeatures feature)
{
    switch (feature)
    {
        case PrinterFeatureRestart:
            return send_gcode("RESTART", false);
        case PrinterFeatureFirmwareRestart:
            return send_gcode("FIRMWARE_RESTART", false);
        case PrinterFeatureHome:
            return send_gcode("G28");
        case PrinterFeatureDisableSteppers:
            return send_gcode("M18");
        case PrinterFeaturePause:
            return send_gcode("PAUSE");
        case PrinterFeatureResume:
            return send_gcode("RESUME");
        case PrinterFeatureStop:
            return send_gcode("CANCEL_PRINT");
        case PrinterFeatureEmergencyStop:
            LOG_LN("Sending estop");
            send_gcode("M112", false);
            send_emergency_stop();

            return true;
        case PrinterFeatureExtrude:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            if (printer_config->custom_filament_move_macros)
            {
                return send_gcode("FILAMENT_EXTRUDE");
            }
            else 
            {
                return send_gcode("M83\nG1 E25 F300");
            }
        case PrinterFeatureRetract:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            if (get_current_printer()->printer_config->custom_filament_move_macros)
            {
                return send_gcode("FILAMENT_RETRACT");
            }
            else 
            {
                return send_gcode("M83\nG1 E-25 F300");
            }
        case PrinterFeatureCooldown:
            return send_gcode("M104 S0\nM140 S0");
        default:
            LOG_F(("Unsupported printer feature %d", feature));
    }

    return false;
}

bool KlipperPrinter::connect()
{
    return connection_test_klipper(printer_config) == KlipperConnectionStatus::ConnectOk;
}

bool KlipperPrinter::fetch()
{
    HTTPClient client;
    configure_http_client(client, "/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan&display_status", true, 1000);

    int http_code = client.GET();
    if (http_code == 200)
    {
        if (printer_data.state == PrinterStateOffline)
        {
            printer_data.state = PrinterStateError;
        }

        klipper_request_consecutive_fail_count = 0;
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        parse_state(doc);
    }
    else
    {
        klipper_request_consecutive_fail_count++;
        LOG_F(("Failed to fetch printer data: %d\n", http_code));

        if (klipper_request_consecutive_fail_count >= 5) 
        {
            printer_data.state = PrinterStateOffline;
            return false;
        }
    }

    return true;
}

PrinterDataMinimal KlipperPrinter::fetch_min()
{
    PrinterDataMinimal data = {};
    data.success = false;

    if (!printer_config->setup_complete)
    {
        data.state = PrinterStateOffline;
        return data;
    }

    data.success = true;

    HTTPClient client;
    configure_http_client(client, "/printer/objects/query?webhooks&print_stats&virtual_sdcard", true, 1000);

    int http_code = client.GET();
    if (http_code == 200)
    {
        data.state = PrinterState::PrinterStateIdle;
        data.power_devices = get_power_devices_count();

        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        return parse_state_min(doc);
    }
    else 
    {
        data.state = PrinterState::PrinterStateOffline;
        data.power_devices = get_power_devices_count();
    }

    return data;
}

void KlipperPrinter::disconnect()
{
    // Nothing to disconnect, everything is http request based
    printer_data.state = PrinterStateOffline;
}

Macros KlipperPrinter::get_macros()
{
    HTTPClient client;
    Macros macros = {0};

    configure_http_client(client, "/printer/gcode/help", true, 1000);
    int http_code = client.GET();

    if (http_code == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        return parse_macros(doc);
    }

    return macros;
}

int KlipperPrinter::get_macros_count()
{
    HTTPClient client;
    configure_http_client(client, "/printer/gcode/help", true, 1000);

    int http_code = client.GET();

    if (http_code == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        return parse_macros_count(doc);
    }
    else {
        return 0;
    }
}

bool KlipperPrinter::execute_macro(const char* macro)
{
    return send_gcode(macro);
}

PowerDevices KlipperPrinter::get_power_devices()
{
    HTTPClient client;
    PowerDevices power_devices = {0};
    configure_http_client(client, "/machine/device_power/devices", true, 1000);

    int http_code = client.GET();

    if (http_code == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        return parse_power_devices(doc);
    }

    return power_devices;
}

int KlipperPrinter::get_power_devices_count()
{
    HTTPClient client;
    configure_http_client(client, "/machine/device_power/devices", true, 1000);

    int http_code = client.GET();

    if (http_code == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        return parse_power_devices_count(doc);
    }
    else {
        return 0;
    }
}

bool KlipperPrinter::set_power_device_state(const char* device_name, bool state)
{
    HTTPClient client;
    configure_http_client(client, "/machine/device_power/device?device=" + urlEncode(device_name) + "&action=" + (state ? "on" : "off"), true, 1000);
    return client.POST("") == 200;
}

#define KLIPPER_FILE_FETCH_LIMIT 20

Files KlipperPrinter::get_files()
{
    Files files_result = {0};
    HTTPClient client;
    LOG_F(("Heap space pre-file-parse: %d bytes\n", esp_get_free_heap_size()))
    std::list<FileSystemFile> files;

    auto timer_request = millis();
    configure_http_client(client, "/server/files/list", true, 5000);

    int http_code = client.GET();
    auto timer_parse = millis();

    if (http_code == 200){
        JsonDocument doc;
        auto parseResult = deserializeJson(doc, client.getStream());
        LOG_F(("Json parse: %s\n", parseResult.c_str()))
        parse_file_list(doc, files, KLIPPER_FILE_FETCH_LIMIT);
    }

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

bool KlipperPrinter::start_file(const char *filename)
{
    HTTPClient client;
    configure_http_client(client, "/printer/print/start?filename=" + urlEncode(filename), false, 1000);

    int http_code = client.POST("");
    LOG_F(("Print start: HTTP %d\n", http_code))
    return http_code == 200;
}

bool KlipperPrinter::set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature)
{
    char gcode[64] = {0};

    switch (device)
    {
        case PrinterTemperatureDeviceBed:
            sprintf(gcode, "M140 S%d", temperature);
            break;
        case PrinterTemperatureDeviceNozzle1:
            sprintf(gcode, "M104 S%d", temperature);
            break;
        default:
            LOG_F(("Unknown temperature device %d was requested to heat to %.2f", device, temperature));
            return false;
    }

    return send_gcode(gcode);
}

Thumbnail KlipperPrinter::get_32_32_png_image_thumbnail(const char* gcode_filename)
{
    Thumbnail thumbnail = {0};
    HTTPClient client;
    configure_http_client(client, "/server/files/thumbnails?filename=" + urlEncode(gcode_filename), true, 1000);
    char* img_filename_path = NULL;
    unsigned char* data_png = NULL;

    int http_code = 0;
    try 
    {
        http_code = client.GET();
    }
    catch (...)
    {
        LOG_LN("Exception while fetching gcode img location");
        return thumbnail;
    }

    if (http_code == 200)
    {
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        img_filename_path = parse_thumbnails(doc);
    }
    else 
    {
        LOG_F(("Failed to fetch gcode image data: %d\n", http_code))
    }

    if (img_filename_path == NULL)
    {
        LOG_LN("No compatible thumbnail found");
        return thumbnail;
    }
    else
    {
        LOG_F(("Found 32x32 PNG gcode img at %s\n", gcode_filename));
    }

    client.end();

    configure_http_client(client, "/server/files/gcodes/" + urlEncode(img_filename_path), false, 2000);

    http_code = 0;
    try 
    {
        http_code = client.GET();
    }
    catch (...)
    {
        LOG_LN("Exception while fetching gcode img");
        return thumbnail;
    }

    if (http_code == 200)
    {
        size_t len = client.getSize();
        if (len <= 0)
        {
            LOG_LN("No gcode img data");
            return thumbnail;
        }

        data_png = (unsigned char*)malloc(len + 1);

        if (data_png != NULL)
        {
            if (len != client.getStream().readBytes(data_png, len))
            {
                LOG_LN("Failed to read gcode img data");
                free(data_png);
            }
            else 
            {
                thumbnail.png = data_png;
                thumbnail.size = len;
                thumbnail.success = true;
            }
        }
    }

    free(img_filename_path);
    return thumbnail;
}

KlipperConnectionStatus connection_test_klipper(PrinterConfiguration* config)
{
    HTTPClient client;

    client.setTimeout(1000);
    client.setConnectTimeout(1000);
    client.begin("http://" + String(config->klipper_host) + ":" + String(config->klipper_port) + "/printer/info");

    if (config->auth_configured) {
        client.addHeader("X-Api-Key", config->klipper_auth);
    }

    int http_code;
    try {
        http_code = client.GET();

        if (http_code == 403)
        {
            return KlipperConnectionStatus::ConnectAuthRequired;
        }

        return http_code == 200 ? KlipperConnectionStatus::ConnectOk : KlipperConnectionStatus::ConnectFail;
    }
    catch (...) {
        LOG_LN("Failed to connect");
        return KlipperConnectionStatus::ConnectFail;
    }
}