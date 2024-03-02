#include "gcode_img.h"
#include "lvgl.h"
#include "ui_utils.h"
#include <esp.h>
#include <ArduinoJson.h>
#include "../conf/global_config.h"
#include <HTTPClient.h>

static unsigned char * data_png = NULL;
static char img_filename_path[256] = {0};
static lv_img_dsc_t img_header = {0};

static void close_button_click(lv_event_t * e){
    lv_obj_t * root = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_del(root);
    clear_img_mem();
}

bool has_128_128_gcode(const char* filename)
{
    if (filename == NULL){
        Serial.println("No gcode filename");
        return false;
    }

    String url = "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + "/server/files/thumbnails?filename=" + String(filename);
    HTTPClient client;
    int httpCode = 0;
    try {
        client.begin(url.c_str());
        httpCode = client.GET();
    }
    catch (...){
        Serial.println("Exception while fetching gcode img location");
        return {0};
    }

    if (httpCode == 200)
    {
        String payload = client.getString();
        JsonDocument doc;
        deserializeJson(doc, payload);
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
            Serial.printf("Found 128x128 PNG gcode img at %s\n", filename);
            strcpy(img_filename_path, chosen_thumb);
            return true;
        }
    }

    return false;
}

lv_obj_t* draw_gcode_img()
{
    clear_img_mem();

    if (img_filename_path[0] == 0){
        Serial.println("No gcode img path");
        return NULL;
    }

    HTTPClient client;
    int httpCode = 0;
    try {
        String img_url = "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + "/server/files/gcodes/" + String(img_filename_path);
        client.begin(img_url);
        httpCode = client.GET();
    }
    catch (...){
        Serial.println("Exception while fetching gcode img");
        return NULL;
    }

    if (httpCode == 200)
    {
        size_t len = client.getSize();
        if (len <= 0)
        {
            Serial.println("No gcode img data");
            return NULL;
        }

        data_png = (unsigned char*)malloc(len + 1);
        if (len != client.getStream().readBytes(data_png, len)){
            Serial.println("Failed to read gcode img data");
            clear_img_mem();
            return NULL;
        }

        Serial.println((char*)data_png);

        memset(&img_header, 0, sizeof(img_header));
        img_header.header.w = 32;
        img_header.header.h = 32;
        img_header.data_size = len;
        img_header.header.cf = LV_IMG_CF_RAW_ALPHA;
        img_header.data = data_png;

        lv_obj_t * img = lv_img_create(lv_scr_act());
        lv_img_set_src(img, &img_header);
        //lv_img_set_zoom(img, 512);

        return img;
    }

    return NULL;
}

lv_obj_t* show_gcode_img(const char* filename)
{
    if (filename == NULL){
        Serial.println("No gcode filename");
        return NULL;
    }

    if (!has_128_128_gcode(filename)){
        Serial.println("No 128x128 gcode img found");
        return NULL;
    }

    return draw_gcode_img();
}

void clear_img_mem()
{
    if (data_png != NULL){
        free(data_png);
        data_png = NULL;
    }
}