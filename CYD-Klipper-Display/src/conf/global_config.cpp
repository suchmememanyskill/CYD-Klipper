#include <Preferences.h>
#include "global_config.h"
#include "lvgl.h"

GLOBAL_CONFIG global_config = {0};

COLOR_DEF color_defs[] = {
    {LV_PALETTE_BLUE, LV_PALETTE_RED},
    {LV_PALETTE_GREEN, LV_PALETTE_PURPLE},
    {LV_PALETTE_GREY, LV_PALETTE_CYAN},
    {LV_PALETTE_YELLOW, LV_PALETTE_PINK},
    {LV_PALETTE_ORANGE, LV_PALETTE_BLUE},
    {LV_PALETTE_RED, LV_PALETTE_GREEN},
    {LV_PALETTE_PURPLE, LV_PALETTE_GREY},
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
    VerifyVersion();
    Preferences preferences;
    preferences.begin("global_config", true);
    preferences.getBytes("global_config", &global_config, sizeof(global_config));
    preferences.end();
}