#include "gcode_img.h"
#include "lvgl.h"
#include "ui_utils.h"
#include <Esp.h>
#include <UrlEncode.h>
#include <ArduinoJson.h>
#include "../conf/global_config.h"
#include "../core/http_client.h"

static unsigned char * data_png = NULL;
static char img_filename_path[256] = {0};
static lv_img_dsc_t img_header = {0};

bool has_32_32_gcode_img(const char* filename)
{
    if (filename == NULL){
        LOG_LN("No gcode filename");
        return false;
    }

    SETUP_HTTP_CLIENT("/server/files/thumbnails?filename=" + urlEncode(filename));
    int httpCode = 0;
    try {
        httpCode = client.GET();
    }
    catch (...){
        LOG_LN("Exception while fetching gcode img location");
        return false;
    }

    if (httpCode == 200)
    {
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
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
            LOG_F(("Found 32x32 PNG gcode img at %s\n", filename))
            strcpy(img_filename_path, chosen_thumb);
            return true;
        }
    }
    else 
    {
        LOG_F(("Failed to fetch gcode image data: %d\n", httpCode))
    }

    return false;
}

lv_obj_t* draw_gcode_img()
{
    clear_img_mem();

    if (img_filename_path[0] == 0){
        LOG_LN("No gcode img path");
        return NULL;
    }

    SETUP_HTTP_CLIENT_FULL("/server/files/gcodes/" + urlEncode(img_filename_path), false, 2000);

    int httpCode = 0;
    try {
        httpCode = client.GET();
    }
    catch (...){
        LOG_LN("Exception while fetching gcode img");
        return NULL;
    }

    if (httpCode == 200)
    {
        size_t len = client.getSize();
        if (len <= 0)
        {
            LOG_LN("No gcode img data");
            return NULL;
        }

        data_png = (unsigned char*)malloc(len + 1);
        if (len != client.getStream().readBytes(data_png, len)){
            LOG_LN("Failed to read gcode img data");
            clear_img_mem();
            return NULL;
        }

        memset(&img_header, 0, sizeof(img_header));
        img_header.header.w = 32;
        img_header.header.h = 32;
        img_header.data_size = len;
        img_header.header.cf = LV_IMG_CF_RAW_ALPHA;
        img_header.data = data_png;

        lv_obj_t * img = lv_img_create(lv_scr_act());
        lv_img_set_src(img, &img_header);

        return img;
    }

    return NULL;
}

lv_obj_t* show_gcode_img(const char* filename)
{
    if (filename == NULL){
        LOG_LN("No gcode filename");
        return NULL;
    }

    if (!has_32_32_gcode_img(filename)){
        LOG_LN("No 32x32 gcode img found");
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