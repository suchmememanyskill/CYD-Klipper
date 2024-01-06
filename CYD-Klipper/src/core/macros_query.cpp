#include "lvgl.h"
#include "macros_query.h"
#include "./data_setup.h"
#include <HTTPClient.h>
#include "../conf/global_config.h"
#include <ArduinoJson.h>

static char* macros[64] = {0};
static int macros_count = 0;

static void on_state_change(void * s, lv_msg_t * m) {
    if (printer.state == PRINTER_STATE_ERROR || printer.state == PRINTER_STATE_PAUSED){
        return;
    }

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

MACROSQUERY macros_query() {
    return {(const char**)macros, macros_count};
}

void macros_query_setup(){
    lv_msg_subscribe(DATA_PRINTER_STATE, on_state_change, NULL);
    on_state_change(NULL, NULL);
}