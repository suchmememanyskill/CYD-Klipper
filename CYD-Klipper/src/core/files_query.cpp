#include <list>
#include "files_query.h"
#include "../conf/global_config.h"
#include "data_setup.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

// Always has +1 entry with a null'd name
FILESYSTEM_FILE* lastQuery = NULL;

FILESYSTEM_FILE* getFiles(int limit){
    freezeRequestThread();

    if (lastQuery != NULL){
        FILESYSTEM_FILE* current = lastQuery;

        while (current->name != NULL){
            free(current->name);
            current += 1;
        }

        free(lastQuery);
    }

    Serial.printf("Heap space pre-file-parse: %d bytes\n", esp_get_free_heap_size());
    std::list<FILESYSTEM_FILE> files;

    auto timerRequest = millis();
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/server/files/list", globalConfig.klipperHost, globalConfig.klipperPort);
    HTTPClient client;
    client.useHTTP10(true);
    client.setTimeout(5000);
    client.begin(buff);

    if (globalConfig.authConfigured)
        client.addHeader("X-Api-Key", globalConfig.klipperAuth);

    int httpCode = client.GET();
    auto timerParse = millis();

    if (httpCode == 200){
        JsonDocument doc;
        auto parseResult = deserializeJson(doc, client.getStream());
        Serial.printf("Json parse: %s\n", parseResult.c_str());
        auto result = doc["result"].as<JsonArray>();

        for (auto file : result){
            FILESYSTEM_FILE f = {0};
            const char* path = file["path"];
            float modified = file["modified"];
            auto fileIter = files.begin();

            while (fileIter != files.end()){
                if ((*fileIter).modified < modified)
                    break;

                fileIter++;
            }

            if (fileIter == files.end() && files.size() >= limit)
                continue;

            f.name = (char*)malloc(strlen(path) + 1);
            if (f.name == NULL){
                Serial.println("Failed to allocate memory");
                continue;
            }
            strcpy(f.name, path);
            f.modified = modified;

            if (fileIter != files.end())
                files.insert(fileIter, f);
            else
                files.push_back(f);

            if (files.size() > limit){
                auto lastEntry = files.back();

                if (lastEntry.name != NULL)
                    free(lastEntry.name);

                files.pop_back();
            }
        }
    }

    size_t size = sizeof(FILESYSTEM_FILE) * (files.size() + 1);
    FILESYSTEM_FILE* result = (FILESYSTEM_FILE*)malloc(size);

    if (result == NULL){
        Serial.println("Failed to allocate memory");

        for (auto file : files){
            free(file.name);
        }

        unfreezeRequestThread();
        return NULL;
    }

    lastQuery = result;
    result[files.size()].name = NULL;

    for (auto file : files){
        *result = file;
        result += 1;
    }

    Serial.printf("Heap space post-file-parse: %d bytes\n", esp_get_free_heap_size());
    Serial.printf("Got %d files. Request took %dms, parsing took %dms\n", files.size(), timerParse - timerRequest, millis() - timerParse);
    unfreezeRequestThread();
    return lastQuery;
}
