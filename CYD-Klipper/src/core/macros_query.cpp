#include "lvgl.h"
#include "macros_query.h"
#include "./data_setup.h"
#include <HTTPClient.h>
#include "../conf/global_config.h"
#include <ArduinoJson.h>
#include <UrlEncode.h>

static char* macros[64] = {0};
static int macrosCount = 0;

static char* powerDevices[16] = {0};
static bool powerDeviceStates[16] = {0};
static int powerDevicesCount = 0;

static void MacrosQueryInternal(){
    String url = "http://" + String(globalConfig.klipperHost) + ":" + String(globalConfig.klipperPort) + "/printer/gcode/help";
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(url.c_str());

    if (globalConfig.authConfigured)
        client.addHeader("X-Api-Key", globalConfig.klipperAuth);

    int httpCode = client.GET();
    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"].as<JsonObject>();

        for (int i = 0; i < macrosCount; i++){
            free(macros[i]);
        }

        macrosCount = 0;

        for (JsonPair i : result){
            const char *key = i.key().c_str();
            const char *value = i.value().as<String>().c_str();
            if (strcmp(value, "CYD_SCREEN_MACRO") == 0) {
                char* macro = (char*)malloc(strlen(key) + 1);
                strcpy(macro, key);
                macros[macrosCount++] = macro;
            }
        }
    }
}

void PowerDevicesClear(){
    for (int i = 0; i < powerDevicesCount; i++){
        free(powerDevices[i]);
    }

    powerDevicesCount = 0;
}

void PowerDevicesQueryInternal(){
    String url = "http://" + String(globalConfig.klipperHost) + ":" + String(globalConfig.klipperPort) + "/machine/device_power/devices";
    HTTPClient client;
    client.useHTTP10(true);
    client.setTimeout(500);
    client.setConnectTimeout(1000);
    client.begin(url.c_str());

    if (globalConfig.authConfigured)
        client.addHeader("X-Api-Key", globalConfig.klipperAuth);

    int httpCode = client.GET();
    if (httpCode == 200){
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto result = doc["result"]["devices"].as<JsonArray>();

        PowerDevicesClear();

        for (auto i : result){
            const char * deviceName = i["device"];
            const char * deviceState = i["status"];
            powerDevices[powerDevicesCount] = (char*)malloc(strlen(deviceName) + 1);
            strcpy(powerDevices[powerDevicesCount], deviceName);
            powerDeviceStates[powerDevicesCount] = strcmp(deviceState, "on") == 0;
            powerDevicesCount++;
        }
    }
}

static void OnStateChange(void * s, lv_msg_t * m) {
    if (printer.state == PRINTER_STATE_ERROR || printer.state == PRINTER_STATE_PAUSED){
        return;
    }

    MacrosQueryInternal();
    PowerDevicesQueryInternal();
}

bool SetPowerState(const char* deviceName, bool state) {
    String url = "http://" + String(globalConfig.klipperHost) + ":" + String(globalConfig.klipperPort) + "/machine/device_power/device?device=" + urlEncode(deviceName) + "&action=" + (state ? "on" : "off");
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(url.c_str());

    if (globalConfig.authConfigured)
        client.addHeader("X-Api-Key", globalConfig.klipperAuth);

    if (client.POST("") != 200)
        return false;

    for (int i = 0; i < powerDevicesCount; i++){
        if (strcmp(powerDevices[i], deviceName) == 0){
            powerDeviceStates[i] = state;
            return true;
        }
    }

    return true;
}

MacrosQueryT MacrosQuery() {
    return {(const char**)macros, (unsigned int)macrosCount};
}

PowerQueryT PowerDevicesQuery() {
    return {(const char**)powerDevices, (const bool*)powerDeviceStates, (unsigned int)powerDevicesCount};
}

void MacrosQuerySetup(){
    lv_msg_subscribe(DATA_PRINTER_STATE, OnStateChange, NULL);
    OnStateChange(NULL, NULL);
}
