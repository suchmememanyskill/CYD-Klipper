#include "lvgl.h"
#include "macros_query.h"
#include "./data_setup.h"
#include <HTTPClient.h>
#include "../conf/global_config.h"
#include <ArduinoJson.h>
#include <UrlEncode.h>

static char* macros[64] = {0};
static int macros_count = 0;

static char* power_devices[16] = {0};
static bool power_device_states[16] = {0};
static int power_devices_count = 0;

static void _macros_query_internal(){
    String url = "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + "/printer/gcode/help";
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(url.c_str());
    int httpCode = client.GET();
    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"].as<JsonObject>();

        for (int i = 0; i < macros_count; i++){
            free(macros[i]);
        }

        macros_count = 0;

        for (JsonPair i : result){
            const char *key = i.key().c_str();
            const char *value = i.value().as<String>().c_str();
            if (strcmp(value, "CYD_SCREEN_MACRO") == 0) {
                char* macro = (char*)malloc(strlen(key) + 1);
                strcpy(macro, key);
                macros[macros_count++] = macro;
            }
        }
    }
}

void power_devices_clear(){
    for (int i = 0; i < power_devices_count; i++){
        free(power_devices[i]);
    }

    power_devices_count = 0;
}

void _power_devices_query_internal(){
    String url = "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + "/machine/device_power/devices";
    HTTPClient client;
    client.useHTTP10(true);
    client.setTimeout(500);
    client.setConnectTimeout(1000);
    client.begin(url.c_str());
    int httpCode = client.GET();
    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"]["devices"].as<JsonArray>();

        power_devices_clear();

        for (auto i : result){
            const char * device_name = i["device"];
            const char * device_state = i["status"];
            power_devices[power_devices_count] = (char*)malloc(strlen(device_name) + 1);
            strcpy(power_devices[power_devices_count], device_name);
            power_device_states[power_devices_count] = strcmp(device_state, "on") == 0;
            power_devices_count++;
        }
    }
}

static void on_state_change(void * s, lv_msg_t * m) {
    if (printer.state == PRINTER_STATE_ERROR || printer.state == PRINTER_STATE_PAUSED){
        return;
    }

    _macros_query_internal();
    _power_devices_query_internal();
}

bool set_power_state(const char* device_name, bool state) {
    String url = "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + "/machine/device_power/device?device=" + urlEncode(device_name) + "&action=" + (state ? "on" : "off");
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(url.c_str());
    if (client.POST("") != 200)
        return false;

    for (int i = 0; i < power_devices_count; i++){
        if (strcmp(power_devices[i], device_name) == 0){
            power_device_states[i] = state;
            return true;
        }
    }

    return true;
}

MACROSQUERY macros_query() {
    return {(const char**)macros, (unsigned int)macros_count};
}

POWERQUERY power_devices_query() {
    return {(const char**)power_devices, (const bool*)power_device_states, (unsigned int)power_devices_count};
}

void macros_query_setup(){
    lv_msg_subscribe(DATA_PRINTER_STATE, on_state_change, NULL);
    on_state_change(NULL, NULL);
}