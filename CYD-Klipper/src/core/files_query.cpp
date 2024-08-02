#include <list>
#include "files_query.h"
#include "../conf/global_config.h"
#include "data_setup.h"
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include "http_client.h"

// Always has +1 entry with a null'd name
FILESYSTEM_FILE* last_query = NULL;

void clear_files()
{
    if (last_query != NULL){
        FILESYSTEM_FILE* current = last_query;

        while (current->name != NULL){
            free(current->name);
            current += 1;
        }

        free(last_query);
        last_query = NULL;
    }
}

FILESYSTEM_FILE* get_files(int limit)
{
    freeze_request_thread();
    clear_files();

    LOG_F(("Heap space pre-file-parse: %d bytes\n", esp_get_free_heap_size()))
    std::list<FILESYSTEM_FILE> files;

    auto timer_request = millis();
    SETUP_HTTP_CLIENT_FULL("/server/files/list", true, 5000);

    int httpCode = client.GET();
    auto timer_parse = millis();

    if (httpCode == 200){
        JsonDocument doc;
        auto parseResult = deserializeJson(doc, client.getStream());
        LOG_F(("Json parse: %s\n", parseResult.c_str()))
        auto result = doc["result"].as<JsonArray>();

        for (auto file : result){
            FILESYSTEM_FILE f = {0};
            const char* path = file["path"];
            float modified = file["modified"];
            auto file_iter = files.begin();

            while (file_iter != files.end()){
                if ((*file_iter).modified < modified)
                    break;

                file_iter++;
            }

            if (file_iter == files.end() && files.size() >= limit)
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

            if (files.size() > limit){
                auto last_entry = files.back();

                if (last_entry.name != NULL)
                    free(last_entry.name);

                files.pop_back();
            }
        }
    }

    size_t size = sizeof(FILESYSTEM_FILE) * (files.size() + 1);
    FILESYSTEM_FILE* result = (FILESYSTEM_FILE*)malloc(size);

    if (result == NULL){
        LOG_LN("Failed to allocate memory");

        for (auto file : files){
            free(file.name);
        }

        unfreeze_request_thread();
        return NULL;
    }

    last_query = result;
    result[files.size()].name = NULL;

    for (auto file : files){
        *result = file;
        result += 1;
    }

    LOG_F(("Heap space post-file-parse: %d bytes\n", esp_get_free_heap_size()))
    LOG_F(("Got %d files. Request took %dms, parsing took %dms\n", files.size(), timer_parse - timer_request, millis() - timer_parse))
    unfreeze_request_thread();
    return last_query;
}