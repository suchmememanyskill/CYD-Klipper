#include <list>
#include "files_query.h"
#include "../conf/global_config.h"
#include "data_setup.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// Always has +1 entry with a null'd name
FILESYSTEM_FILE* last_query = NULL;

FILESYSTEM_FILE* get_files(){
    freeze_request_thread();

    if (last_query != NULL){
        FILESYSTEM_FILE* current = last_query;

        while (current->name != NULL){
            free(current->name);
            current += 1;
        }

        free(last_query);
    }

    std::list<FILESYSTEM_FILE> files;
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/server/files/list", global_config.klipperHost, global_config.klipperPort);
    HTTPClient client;
    client.begin(buff);
    int httpCode = client.GET();
    int count = 0;
    if (httpCode == 200){
        String payload = client.getString();
        DynamicJsonDocument doc(60000);
        auto a = deserializeJson(doc, payload);
        Serial.printf("JSON PARSE: %s\n", a.c_str());
        auto result = doc["result"].as<JsonArray>();
        for (auto file : result){
            FILESYSTEM_FILE f = {0};
            const char* path = file["path"];
            f.name = (char*)malloc(strlen(path) + 1);
            strcpy(f.name, path);
            f.modified = file["modified"];
            files.push_back(f);
            count++;
        }
    }

    //Serial.printf("Found %d files\n", count);
    files.sort([](FILESYSTEM_FILE a, FILESYSTEM_FILE b){return a.modified < b.modified;});
    files.reverse(); // TODO: Reverse is unneeded here, we can iterate backwards

    size_t size = sizeof(FILESYSTEM_FILE) * (files.size() + 1);
    FILESYSTEM_FILE* result = (FILESYSTEM_FILE*)malloc(size);
    //Serial.printf("Allocated %d bytes\n", size);
    last_query = result;
    result[files.size()].name = NULL;

    for (auto file : files){
        *result = file;
        result += 1;
    }

    unfreeze_request_thread();
    return last_query;
}