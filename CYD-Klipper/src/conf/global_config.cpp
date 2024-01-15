#include <Preferences.h>
#include "global_config.h"
#include "lvgl.h"

GLOBAL_CONFIG global_config = {0};

COLOR_DEF color_defs[] = {
    {LV_PALETTE_BLUE, 0, LV_PALETTE_RED},
    {LV_PALETTE_LIME, -2, LV_PALETTE_PURPLE},
    {LV_PALETTE_GREY, 0, LV_PALETTE_CYAN},
    {LV_PALETTE_YELLOW, -2, LV_PALETTE_PINK},
    {LV_PALETTE_ORANGE, -2, LV_PALETTE_BLUE},
    {LV_PALETTE_RED, 0, LV_PALETTE_GREEN},
    {LV_PALETTE_PURPLE, 0, LV_PALETTE_GREY},
};

void WriteGlobalConfig() {
    Preferences preferences;
    preferences.begin("global_config", false);
    preferences.putBytes("global_config", &global_config, sizeof(global_config));
    preferences.end();
}

void VerifyVersion(){
    Preferences preferences;
    if (!preferences.begin("global_config", false))
        return;
    
    GLOBAL_CONFIG config = {0};
    preferences.getBytes("global_config", &config, sizeof(config));
    Serial.printf("Config version: %d\n", config.version);
    if (config.version != CONFIG_VERSION) {
        Serial.println("Clearing Global Config");
        preferences.clear();
    }

    preferences.end();
}

void LoadGlobalConfig() {
    global_config.version = CONFIG_VERSION;
    global_config.brightness = 255;
    global_config.screenTimeout = 5;
    global_config.hotend_presets[0] = 0;
    global_config.hotend_presets[1] = 200;
    global_config.hotend_presets[2] = 240;
    global_config.bed_presets[0] = 0;
    global_config.bed_presets[1] = 60;
    global_config.bed_presets[2] = 70;
    VerifyVersion();
    Preferences preferences;
    preferences.begin("global_config", true);
    preferences.getBytes("global_config", &global_config, sizeof(global_config));
    preferences.end();
}