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
    int time_estimate_s = doc["result"]["estimated_time"];
    LOG_F(("Got slicer time estimate: %ds\n", time_estimate_s))
    return time_estimate_s;
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

bool KlipperPrinter::execute_feature(PrinterFeatures feature)
{
    HTTPClient client;

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
            configure_http_client(client, "/printer/emergency_stop", false, 5000);

            try
            {
                client.GET();
            }
            catch (...)
            {
                LOG_LN("Failed to send estop");
            }

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

            if (get_current_printer_config()->custom_filament_move_macros)
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
    HTTPClient client;
    configure_http_client(client, "/printer/info", false, 1000);

    int http_code;
    try {
        http_code = client.GET();
        return http_code == 200;
    }
    catch (...) {
        LOG_LN("Failed to connect");
        return false;
    }
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
        auto status = doc["result"]["status"];
        bool emit_state_update = false;
        
        if (status.containsKey("webhooks"))
        {
            const char *state = status["webhooks"]["state"];
            const char *message = status["webhooks"]["state_message"];

            if (strcmp(state, "ready") == 0 && printer_data.state == PrinterStateError)
            {
                printer_data.state = PrinterStateIdle;
            }
            else if ((strcmp(state, "shutdown") == 0 || strcmp(state, "error") == 0) && printer_data.state != PrinterStateError)
            {
                printer_data.state = PrinterStateError;
            }

            if (printer_data.state_message == NULL || strcmp(printer_data.state_message, message))
            {
                printer_data.state_message = (char *)malloc(strlen(message) + 1);
                strcpy(printer_data.state_message, message);
            }
        }

        if (printer_data.state != PrinterStateError)
        {
            if (status.containsKey("extruder"))
            {
                printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] = status["extruder"]["temperature"];
                printer_data.target_temperatures[PrinterTemperatureDeviceIndexNozzle1] = status["extruder"]["target"];
                bool can_extrude = status["extruder"]["can_extrude"];
                printer_data.pressure_advance = status["extruder"]["pressure_advance"];
                printer_data.smooth_time = status["extruder"]["smooth_time"];
                printer_data.can_extrude = can_extrude == true;
            }

            if (status.containsKey("heater_bed"))
            {
                printer_data.temperatures[PrinterTemperatureDeviceIndexBed] = status["heater_bed"]["temperature"];
                printer_data.target_temperatures[PrinterTemperatureDeviceIndexBed] = status["heater_bed"]["target"];
            }

            if (status.containsKey("toolhead"))
            {
                const char *homed_axis = status["toolhead"]["homed_axes"];
                printer_data.homed_axis = strcmp(homed_axis, "xyz") == 0;
            }

            if (status.containsKey("gcode_move"))
            {
                printer_data.position[0] = status["gcode_move"]["gcode_position"][0];
                printer_data.position[1] = status["gcode_move"]["gcode_position"][1];
                printer_data.position[2] = status["gcode_move"]["gcode_position"][2];
                gcode_offset[0] = status["gcode_move"]["homing_origin"][0];
                gcode_offset[1] = status["gcode_move"]["homing_origin"][1];
                gcode_offset[2] = status["gcode_move"]["homing_origin"][2];
                bool absolute_coords = status["gcode_move"]["absolute_coordinates"];

                if (lock_absolute_relative_mode_swap > 0)
                {
                    lock_absolute_relative_mode_swap--;
                }
                else 
                {
                    printer_data.absolute_coords = absolute_coords == true;
                }

                printer_data.speed_mult = status["gcode_move"]["speed_factor"];
                printer_data.extrude_mult = status["gcode_move"]["extrude_factor"];
                printer_data.feedrate_mm_per_s = status["gcode_move"]["speed"];
                printer_data.feedrate_mm_per_s /= 60; // convert mm/m to mm/s
            }

            if (status.containsKey("fan"))
            {
                printer_data.fan_speed = status["fan"]["speed"];
            }

            if (status.containsKey("virtual_sdcard"))
            {
                printer_data.print_progress = status["virtual_sdcard"]["progress"];
            }

            if (status.containsKey("print_stats"))
            {
                const char *filename = status["print_stats"]["filename"];

                if (filename != NULL && (printer_data.print_filename == NULL || strcmp(printer_data.print_filename, filename)))
                {
                    printer_data.print_filename = (char *)malloc(strlen(filename) + 1);
                    strcpy(printer_data.print_filename, filename);
                }

                printer_data.elapsed_time_s = status["print_stats"]["total_duration"];
                printer_data.printed_time_s = status["print_stats"]["print_duration"];
                printer_data.filament_used_mm = status["print_stats"]["filament_used"];
                printer_data.total_layers = status["print_stats"]["info"]["total_layer"];
                printer_data.current_layer = status["print_stats"]["info"]["current_layer"];

                const char *state = status["print_stats"]["state"];

                if (state == nullptr)
                {
                    // Continue
                }
                else if (strcmp(state, "printing") == 0)
                {
                    printer_data.state = PrinterStatePrinting;
                }
                else if (strcmp(state, "paused") == 0)
                {
                    printer_data.state = PrinterStatePaused;
                }
                else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
                {
                    printer_data.state = PrinterStateIdle;
                }
            }

            if (status.containsKey("display_status"))
            {
                printer_data.print_progress = status["display_status"]["progress"];
                const char* message = status["display_status"]["message"];

                if (message != NULL && (printer_data.popup_message == NULL || strcmp(printer_data.popup_message, message)))
                {
                    printer_data.popup_message = (char*)malloc(strlen(message) + 1);
                    strcpy(printer_data.popup_message, message);
                }
            }

            if (printer_data.state == PrinterStatePrinting && printer_data.print_progress > 0)
            {
                float remaining_time_s_percentage = (printer_data.printed_time_s / printer_data.print_progress) - printer_data.printed_time_s;
                float remaining_time_s_slicer = 0;

                if (slicer_estimated_print_time_s > 0)
                {
                    remaining_time_s_slicer = slicer_estimated_print_time_s - printer_data.printed_time_s;
                }

                if (remaining_time_s_slicer <= 0 || printer_config->remaining_time_calc_mode == REMAINING_TIME_CALC_PERCENTAGE)
                {
                    printer_data.remaining_time_s = remaining_time_s_percentage;
                }
                else if (printer_config->remaining_time_calc_mode == REMAINING_TIME_CALC_INTERPOLATED)
                {
                    printer_data.remaining_time_s = remaining_time_s_percentage * printer_data.print_progress + remaining_time_s_slicer * (1 - printer_data.print_progress);
                }
                else if (printer_config->remaining_time_calc_mode == REMAINING_TIME_CALC_SLICER)
                {
                    printer_data.remaining_time_s = remaining_time_s_slicer;
                }
            }

            if (printer_data.remaining_time_s < 0)
            {
                printer_data.remaining_time_s = 0;
            }

            if (printer_data.state == PrinterStateIdle)
            {
                slicer_estimated_print_time_s = 0;
            }
        }
        
        if (printer_data.state == PrinterStatePrinting && millis() - last_slicer_time_query > 30000 && slicer_estimated_print_time_s <= 0)
        {
            last_slicer_time_query = millis();
            slicer_estimated_print_time_s = get_slicer_time_estimate_s();
        }
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

int KlipperPrinter::get_power_devices_count()
{
    return 0;
}

PrinterDataMinimal KlipperPrinter::fetch_min()
{
    PrinterDataMinimal data = {0};

    if (!printer_config->ip_configured)
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
        data.state = PrinterStateIdle;
        data.power_devices = get_power_devices_count();

        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto status = doc["result"]["status"];

        if (status.containsKey("webhooks"))
        {
            const char *state = status["webhooks"]["state"];

            if (strcmp(state, "shutdown") == 0)
            {
                data.state = PrinterStateError;
            }
        }

        if (data.state != PrinterStateError)
        {
            if (status.containsKey("virtual_sdcard"))
            {
                data.print_progress = status["virtual_sdcard"]["progress"];
            }

            if (status.containsKey("print_stats"))
            {
                const char *state = status["print_stats"]["state"];

                if (state == nullptr)
                {
                    data.state = PrinterStateError;
                }
                else if (strcmp(state, "printing") == 0)
                {
                    data.state = PrinterStatePrinting;
                }
                else if (strcmp(state, "paused") == 0)
                {
                    data.state = PrinterStatePaused;
                }
                else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
                {
                    data.state = PrinterStateIdle;
                }
            }
        }
    }
    else 
    {
        data.state = PrinterStateOffline;
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
    Macros macros;

    configure_http_client(client, "/printer/gcode/help", true, 1000);
    int http_code = client.GET();

    if (http_code == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"].as<JsonObject>();
        macros.macros = (char**)malloc(sizeof(char*) * 32);
        macros.count = 0;
        macros.success = true;

        for (JsonPair i : result){
            const char *key = i.key().c_str();
            const char *value = i.value().as<String>().c_str();
            if (strcmp(value, "CYD_SCREEN_MACRO") == 0) {
                char* macro = (char*)malloc(strlen(key) + 1);
                strcpy(macro, key);
                macros.macros[macros.count++] = macro;
            }
        }

        if (global_config->sort_macros)
        {
            std::sort(macros.macros, macros.macros + macros.count, [](const char* a, const char* b) {
                return strcmp(a, b) < 0;
            });
        }
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
        auto result = doc["result"].as<JsonObject>();

        int count = 0;

        for (JsonPair i : result){
            const char *value = i.value().as<String>().c_str();
            if (strcmp(value, "CYD_SCREEN_MACRO") == 0) {
                count++;
            }
        }

        return count;
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
    PowerDevices power_devices;
    configure_http_client(client, "/machine/device_power/devices", true, 1000);

    int http_code = client.GET();

    if (http_code == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"]["devices"].as<JsonArray>();
        power_devices.power_devices = (char**)malloc(sizeof(char*) * 16);
        power_devices.power_states = (bool*)malloc(sizeof(bool) * 16);
        power_devices.count = 0;
        power_devices.success = true;

        for (auto i : result){
            const char * device_name = i["device"];
            const char * device_state = i["status"];
            power_devices.power_devices[power_devices.count] = (char*)malloc(strlen(device_name) + 1);
            strcpy(power_devices.power_devices[power_devices.count], device_name);
            power_devices.power_states[power_devices.count] = strcmp(device_state, "on") == 0;
            power_devices.count++;
        }
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
        auto result = doc["result"]["devices"].as<JsonArray>();

        int count = 0;

        for (auto i : result){
            count++;
        }

        return count;
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

typedef struct {
    char* name;
    float modified;
} FileSystemFile;

#define KLIPPER_FILE_FETCH_LIMIT 20

Files KlipperPrinter::get_files()
{
    Files files_result;
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
        auto result = doc["result"].as<JsonArray>();

        for (auto file : result){
            FileSystemFile f = {0};
            const char* path = file["path"];
            float modified = file["modified"];
            auto file_iter = files.begin();

            while (file_iter != files.end()){
                if ((*file_iter).modified < modified)
                    break;

                file_iter++;
            }

            if (file_iter == files.end() && files.size() >= KLIPPER_FILE_FETCH_LIMIT)
                continue;
            
            f.name = (char*)malloc(strlen(path) + 1);
            if (f.name == NULL){
                LOG_LN("Failed to allocate memory");
                continue;
            }
            strcpy(f.name, path);
            f.modified = modified;

            if (file_iter != files.end())
                files.insert(file_iter, f);
            else 
                files.push_back(f);

            if (files.size() > KLIPPER_FILE_FETCH_LIMIT){
                auto last_entry = files.back();

                if (last_entry.name != NULL)
                    free(last_entry.name);

                files.pop_back();
            }
        }
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

bool KlipperPrinter::set_target_temperature(PrinterTemperatureDevice device, float temperature)
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

unsigned char* KlipperPrinter::get_32_32_png_image_thumbnail(const char* gcode_filename)
{
    HTTPClient client;
    configure_http_client(client, "/server/files/thumbnails?filename=", true, 1000);
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
        return NULL;
    }

    if (http_code == 200)
    {
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"].as<JsonArray>();
        const char* chosen_thumb = NULL;

        for (auto file : result){
            int width = file["width"];
            int height = file["height"];
            int size = file["size"];
            const char* thumbnail = file["thumbnail_path"];

            if (width != height || width != 32)
                continue;

            if (strcmp(thumbnail + strlen(thumbnail) - 4, ".png"))
                continue;

            chosen_thumb = thumbnail;
            break;
        }

        if (chosen_thumb != NULL){
            LOG_F(("Found 32x32 PNG gcode img at %s\n", gcode_filename))
            img_filename_path = (char*)malloc(strlen(chosen_thumb) + 1);
            strcpy(img_filename_path, chosen_thumb);
        }
    }
    else 
    {
        LOG_F(("Failed to fetch gcode image data: %d\n", http_code))
    }

    if (img_filename_path == NULL)
    {
        return NULL;
    }

    client.end();

    configure_http_client(client, "/server/files/gcodes/" + urlEncode(img_filename_path), false, 2000);

    int http_code = 0;
    try 
    {
        http_code = client.GET();
    }
    catch (...)
    {
        LOG_LN("Exception while fetching gcode img");
        return NULL;
    }

    if (http_code == 200)
    {
        size_t len = client.getSize();
        if (len <= 0)
        {
            LOG_LN("No gcode img data");
            return NULL;
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
                free(img_filename_path);
                return data_png;
            }
        }
    }

    free(img_filename_path);
    return NULL;
}