#include "bambu_printer_integration.hpp"
#include <PubSubClient.h>

WiFiClientSecure wifi_client;
PubSubClient client(wifi_client);
BambuPrinter* current_printer = NULL;

const char* COMMAND_FETCH_ALL = "{\"pushing\":{\"sequence_id\":\"0\",\"command\":\"pushall\",\"version\":1,\"push_target\":1}}";
const char* COMMAND_LIGHTCTL = "{\"system\":{\"sequence_id\":\"0\",\"command\":\"ledctrl\",\"led_node\":\"%s\",\"led_mode\":\"%s\"}}";
const char* COMMAND_SEND_GCODE = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"gcode_line\",\"param\":\"%s\"}}";
const char* COMMAND_PRINT_STOP = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"stop\",\"param\":\"\"}}";
const char* COMMAND_PRINT_PAUSE = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"pause\",\"param\":\"\"}}";
const char* COMMAND_PRINT_RESUME = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"resume\",\"param\":\"\"}}";
const char* COMMAND_FILAMENT_UNLOAD = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"unload_filament\"}}";
const char* COMMAND_FILAMENT_LOAD_EXTERNAL = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"ams_change_filament\",\"target\":254,\"curr_temp\":215,\"tar_temp\":250}}";
const char* COMMAND_AMS_CONTOL_DONE = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"ams_control\",\"param\":\"done\"}}";
const char* COMMAND_AMS_CONTOL_RETRY = "{\"print\":{\"sequence_id\":\"0\",\"command\":\"ams_control\",\"param\":\"resume\"}}";

static void callback(char* topic, byte* payload, unsigned int length)
{
    if (current_printer != NULL)
    {
        current_printer->receive_data(payload, length);
    }
}

void BambuPrinter::receive_data(unsigned char* data, unsigned int length)
{
    data[length] = 0;
    JsonDocument doc;
    deserializeJson(doc, data);
    parse_state(doc);
}

bool BambuPrinter::publish_mqtt_command(const char* command)
{
    if (!client.connected())
    {
        return false;
    }

    char auth[48] = {0};
    sprintf(auth, "device/%s/request", printer_config->printer_auth);

    LOG_F(("Publishing MQTT Command: %s\n", command));
    return client.publish(auth, command);
}

bool BambuPrinter::move_printer(const char* axis, float amount, bool relative)
{
    if (!printer_data.homed_axis || printer_data.state == PrinterStatePrinting)
        return false;

    char gcode[64];
    const char* extra = (amount > 0) ? "+" : "";
    const char* start = "";
    const char* end = "";

    if (relative) {
        start = "G91\n";
    }
    else {
        start = "G90\n";
    }

    sprintf(gcode, "%sG1 %s%s%.3f F6000", start, axis, extra, amount);
    send_gcode(gcode);

    return true;
}

bool BambuPrinter::execute_feature(PrinterFeatures feature)
{
    switch (feature)
    {
        case PrinterFeatureHome:
            return send_gcode("G28");
        case PrinterFeatureDisableSteppers:
            return send_gcode("M18 X Y Z");
        case PrinterFeaturePause:
            return publish_mqtt_command(COMMAND_PRINT_PAUSE);
        case PrinterFeatureResume:
            return publish_mqtt_command(COMMAND_PRINT_RESUME);
        case PrinterFeatureStop:
            return publish_mqtt_command(COMMAND_PRINT_STOP);
        case PrinterFeatureExtrude:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            return send_gcode("M83\nG1 E25 F300");
        case PrinterFeatureRetract:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            return send_gcode("M83\nG1 E-25 F300");
        case PrinterFeatureCooldown:
            return send_gcode("M104 S0\nM140 S0");
        case PrinterFeatureContinueError:
            return publish_mqtt_command(COMMAND_AMS_CONTOL_DONE);
        case PrinterFeatureRetryError:
            return publish_mqtt_command(COMMAND_AMS_CONTOL_RETRY);
        case PrinterFeatureIgnoreError:
            ignore_error = last_error;
            publish_mqtt_command(COMMAND_FETCH_ALL);
            return true;
        default:
            LOG_F(("Unsupported printer feature %d", feature));
    }

    return false;
}

bool BambuPrinter::connect()
{
    wifi_client.setInsecure();
    wifi_client.setTimeout(3);
    client.setBufferSize(4096);
    client.setServer(printer_config->printer_host, 8883);
    current_printer = this;
    client.setCallback(NULL);
    char buff[10] = {0};
    sprintf(buff, "%d", printer_config->klipper_port);
    if (!client.connect("id", "bblp", buff))
    {
        LOG_LN("Bambu: Wrong IP or LAN code.");
        return false;
    }

    char auth[48] = {0};
    sprintf(auth, "device/%s/report", printer_config->printer_auth);

    if (!client.subscribe(auth))
    {
        LOG_LN("Bambu: Wrong serial number.");
        return false;
    }

    delay(100);
    client.loop();

    if (!client.connected())
    {
        LOG_LN("Bambu: Connection lost. Likely wrong serial number.");
        return false;
    }

    client.setCallback(callback);
    printer_data.state = PrinterState::PrinterStateIdle;
    return publish_mqtt_command(COMMAND_FETCH_ALL);
}

void BambuPrinter::disconnect()
{
    current_printer = NULL;
    printer_data.state = PrinterState::PrinterStateOffline;
    client.disconnect();
    client.setCallback(NULL);
    client.setBufferSize(16);
}

bool BambuPrinter::fetch()
{
    if (!client.connected())
    {
        LOG_LN("Failed to fetch printer data: Not connected");
        return false;
    }

    if (!client.loop())
    {
        LOG_LN("Failed to fetch printer data: Fetching data failed");
        return false;
    }

    return true;
}

PrinterDataMinimal BambuPrinter::fetch_min()
{
    PrinterDataMinimal min = {};
    min.success = true;
    min.state = PrinterState::PrinterStateIdle;
    min.print_progress = 0;
    min.power_devices = 0;
    return min;
}

const char * MACRO_UNLOAD = "Unload filament";
const char * MACRO_LOAD = "Load filament (External)";

Macros BambuPrinter::get_macros()
{
    Macros macros = {0};
    macros.success = true;
    macros.count = get_macros_count();
    macros.macros = (char **)malloc(sizeof(char *) * macros.count);
    
    macros.macros[0] = (char *)malloc(25);
    strcpy(macros.macros[0], MACRO_LOAD);

    macros.macros[1] = (char *)malloc(16);
    strcpy(macros.macros[1], MACRO_UNLOAD);

    return macros;
}

int BambuPrinter::get_macros_count()
{
    return 2;
}

bool BambuPrinter::execute_macro(const char* macro)
{
    if (strcmp(macro, MACRO_LOAD) == 0)
    {
        return publish_mqtt_command(COMMAND_FILAMENT_LOAD_EXTERNAL);
    }
    else if (strcmp(macro, MACRO_UNLOAD) == 0)
    {
        return publish_mqtt_command(COMMAND_FILAMENT_UNLOAD);
    }

    return false;
}

const char* WORK_LIGHT = "Work Light";
const char* CHAMBER_LIGHT = "Chamber Light";

PowerDevices BambuPrinter::get_power_devices()
{
    PowerDevices power_devices = {0};
    power_devices.success = true;
    int count = get_power_devices_count();

    if (count == 0)
    {
        return power_devices;
    }

    power_devices.power_devices = (char **)malloc(sizeof(char *) * count);
    power_devices.power_states = (bool *)malloc(sizeof(bool) * count);

    if (work_light_available)
    {
        power_devices.power_devices[power_devices.count] = (char *)malloc(10 + 1);
        strcpy(power_devices.power_devices[power_devices.count], WORK_LIGHT);
        power_devices.power_states[power_devices.count] = work_light_on;
        power_devices.count++;
    }

    if (chamber_light_available)
    {
        power_devices.power_devices[power_devices.count] = (char *)malloc(13 + 1);
        strcpy(power_devices.power_devices[power_devices.count], CHAMBER_LIGHT);
        power_devices.power_states[power_devices.count] = chamber_light_on;
        power_devices.count++;
    }

    return power_devices;
}

int BambuPrinter::get_power_devices_count()
{
    return (work_light_available ? 1 : 0) + (chamber_light_available ? 1 : 0);
}

bool BambuPrinter::set_power_device_state(const char* device_name, bool state)
{
    char buff[128] = {0};
    const char* device;

    if (strcmp(device_name, WORK_LIGHT) == 0)
    {
        device = "work_light";
    }
    else if (strcmp(device_name, CHAMBER_LIGHT) == 0)
    {
        device = "chamber_light";
    }
    else 
    {
        return false;
    }

    sprintf(buff, COMMAND_LIGHTCTL, device, state ? "on" : "off");
    return publish_mqtt_command(buff);
}

Files BambuPrinter::get_files()
{
    PrinterState state = printer_data.state;
    disconnect();
    Files files = parse_files(wifi_client, 20);
    connect();
    printer_data.state = state;
    return files;
}

Thumbnail BambuPrinter::get_32_32_png_image_thumbnail(const char* gcode_filename)
{
    Thumbnail thumbnail = {0};
    return thumbnail;
}

bool BambuPrinter::set_target_temperature(PrinterTemperatureDevice device, unsigned int temperature)
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

bool BambuPrinter::send_gcode(const char* gcode, bool wait)
{
    char* buff = (char *)malloc(strlen(gcode) + 70);
    sprintf(buff, COMMAND_SEND_GCODE, gcode);
    return publish_mqtt_command(buff);
}

BambuConnectionStatus connection_test_bambu(PrinterConfiguration* config)
{
    WiFiClientSecure connection_test_wifi_client;
    PubSubClient connection_test_client(connection_test_wifi_client);
    connection_test_wifi_client.setInsecure();
    connection_test_client.setServer(config->printer_host, 8883);
    char buff[10] = {0};
    sprintf(buff, "%d", config->klipper_port);
    if (!connection_test_client.connect("id", "bblp", buff))
    {
        LOG_LN("Bambu: Wrong IP or LAN code.");
        return BambuConnectionStatus::BambuConnectFail;
    }

    char auth[48] = {0};
    sprintf(auth, "device/%s/report", config->printer_auth);

    if (!connection_test_client.subscribe(auth))
    {
        LOG_LN("Bambu: Wrong serial number.");
        return BambuConnectionStatus::BambuConnectSNFail;
    }

    delay(100);
    connection_test_client.loop();

    if (!connection_test_client.connected())
    {
        LOG_LN("Bambu: Connection lost. Likely wrong serial number.");
        return BambuConnectionStatus::BambuConnectSNFail;
    }

    connection_test_client.disconnect();
    LOG_LN("Bambu: Connection test successful!");
    return BambuConnectionStatus::BambuConnectOk;
}