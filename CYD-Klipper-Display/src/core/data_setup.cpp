
#include "data_setup.h"
#include "lvgl.h"
#include "../conf/global_config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char * printer_state_messages[] = {
    "Error",
    "Idle",
    "Printing"
};

Printer printer = {0};

void fetch_printer_state(){
    char buff[91] = {};
    sprintf(buff, "http://%s:%d/printer/info", global_config.klipperHost, global_config.klipperPort);
    HTTPClient client;
    client.begin(buff);
    int httpCode = client.GET();
    if (httpCode == 200) {
        String payload = client.getString();
        Serial.println(payload);
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);
        auto result = doc["result"];

        bool update = false;
        const char* state = result["state"];

        if (strcmp(state, "ready") == 0 && printer.state == PRINTER_STATE_ERROR){
            printer.state = PRINTER_STATE_IDLE;
            update = true;
        }
        else if (strcmp(state, "shutdown") == 0 && printer.state != PRINTER_STATE_ERROR) {
            printer.state = PRINTER_STATE_ERROR;
            update = true;
        }

        const char* message = result["state_message"];
        if (printer.state_message == NULL || strcmp(printer.state_message, message)) {
            if (printer.state_message != NULL){
                free(printer.state_message);
            }

            printer.state_message = (char*)malloc(strlen(message) + 1);
            strcpy(printer.state_message, message);
            update = true;
        }

        if (update)
            lv_msg_send(DATA_PRINTER_STATE, &printer);
    }
    else {
        Serial.printf("Failed to fetch printer state: %d\n", httpCode);
    }
}

void fetch_printer_data(){
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/printer/objects/query?extruder&heater_bed&toolhead", global_config.klipperHost, global_config.klipperPort);
    HTTPClient client;
    client.begin(buff);
    int httpCode = client.GET();
    if (httpCode == 200) {
        String payload = client.getString();
        Serial.println(payload);
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, payload);
        auto status = doc["result"]["status"];
        if (status.containsKey("extruder")){
            printer.extruder_temp = status["extruder"]["temperature"];
            printer.extruder_target_temp = status["extruder"]["target"];
        }

        if (status.containsKey("heater_bed")){
            printer.bed_temp = status["heater_bed"]["temperature"];
            printer.bed_target_temp = status["heater_bed"]["target"];
        }

        if (status.containsKey("toolhead")){
            printer.position[0] = status["toolhead"]["position"][0];
            printer.position[1] = status["toolhead"]["position"][1];
            printer.position[2] = status["toolhead"]["position"][2];
        }

        lv_msg_send(DATA_PRINTER_DATA, &printer);
    }
    else {
        Serial.printf("Failed to fetch printer data: %d\n", httpCode);
    }
}

void data_loop(){
    // TODO: slow down requests
    fetch_printer_state();
    if (printer.state != PRINTER_STATE_ERROR)
        fetch_printer_data();
}

void data_setup(){
    fetch_printer_state();
    fetch_printer_data();
}