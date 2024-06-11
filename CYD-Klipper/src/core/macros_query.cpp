#include "lvgl.h"
#include "macros_query.h"
#include "./data_setup.h"
#include <ArduinoJson.h>
#include <UrlEncode.h>
#include "http_client.h"

static char* macros[64] = {0};
static int macros_count = 0;

static char* power_devices[16] = {0};
static bool power_device_states[16] = {0};
static unsigned int stored_power_devices_count = 0;

void macros_clear()
{
    for (int i = 0; i < macros_count; i++){
        free(macros[i]);
    }

    macros_count = 0;
}

MACROSQUERY macros_query(PRINTER_CONFIG * config) 
{
    HTTPClient client;
    configure_http_client(client, get_full_url("/printer/gcode/help", config), true, 1000);

    int httpCode = client.GET();

    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"].as<JsonObject>();

        macros_clear();

        for (JsonPair i : result){
            const char *key = i.key().c_str();
            const char *value = i.value().as<String>().c_str();
            if (strcmp(value, "CYD_SCREEN_MACRO") == 0) {
                char* macro = (char*)malloc(strlen(key) + 1);
                strcpy(macro, key);
                macros[macros_count++] = macro;
            }
        }

        if (global_config.sort_macros)
        {
            std::sort(macros, macros + macros_count, [](const char* a, const char* b) {
                return strcmp(a, b) < 0;
            });
        }

        return {(const char**)macros, (unsigned int)macros_count};
    }
    else {
        return {NULL, 0};
    }
}

MACROSQUERY macros_query() 
{
    return macros_query(get_current_printer_config());
}

unsigned int macro_count(PRINTER_CONFIG * config) 
{
    HTTPClient client;
    configure_http_client(client, get_full_url("/printer/gcode/help", config), true, 1000);

    int httpCode = client.GET();

    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"].as<JsonObject>();

        unsigned int count = 0;

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

unsigned int macro_count() 
{
    return macro_count(get_current_printer_config());
}

void power_devices_clear()
{
    for (int i = 0; i < stored_power_devices_count; i++){
        free(power_devices[i]);
    }

    stored_power_devices_count = 0;
}

POWERQUERY power_devices_query(PRINTER_CONFIG * config)
{
    HTTPClient client;
    configure_http_client(client, get_full_url("/machine/device_power/devices", config), true, 1000);

    int httpCode = client.GET();

    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"]["devices"].as<JsonArray>();

        power_devices_clear();

        for (auto i : result){
            const char * device_name = i["device"];
            const char * device_state = i["status"];
            power_devices[stored_power_devices_count] = (char*)malloc(strlen(device_name) + 1);
            strcpy(power_devices[stored_power_devices_count], device_name);
            power_device_states[stored_power_devices_count] = strcmp(device_state, "on") == 0;
            stored_power_devices_count++;
        }

        return {(const char**)power_devices, (const bool*)power_device_states, (unsigned int)stored_power_devices_count};
    }
    else {
        return {NULL, NULL, 0};
    }
}

POWERQUERY power_devices_query() 
{
    return power_devices_query(get_current_printer_config());
}

unsigned int power_devices_count(PRINTER_CONFIG * config)
{
    HTTPClient client;
    configure_http_client(client, get_full_url("/machine/device_power/devices", config), true, 1000);

    int httpCode = client.GET();

    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"]["devices"].as<JsonArray>();

        unsigned int count = 0;

        for (auto i : result){
            count++;
        }

        return count;
    }
    else {
        return 0;
    }
}

unsigned int power_devices_count() 
{
    return power_devices_count(get_current_printer_config());
}

bool set_power_state(const char* device_name, bool state, PRINTER_CONFIG * config) 
{
    HTTPClient client;
    configure_http_client(client, get_full_url("/machine/device_power/device?device=" + urlEncode(device_name) + "&action=" + (state ? "on" : "off"), config), true, 1000);
 
    return client.POST("") == 200;
}

bool set_power_state(const char* device_name, bool state) 
{
    return set_power_state(device_name, state, get_current_printer_config());
}