
#include "global_config.h"
#include <Preferences.h>
#include "lvgl.h"
#include <array>
GlobalConfig globalConfig = {0};
ColorDef colorDefs[] = {
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
    preferences.begin(GLOBAL_CONFIG_NAMESPACE, false);
    preferences.putBytes(GLOBAL_CONFIG_KEY, &globalConfig, sizeof(globalConfig));
    preferences.end();
}

void VerifyVersion(){
    Preferences preferences;
    if (!preferences.begin(GLOBAL_CONFIG_NAMESPACE, false))
        return;

    GlobalConfig config = {0};
    preferences.getBytes(GLOBAL_CONFIG_KEY, &config, sizeof(config));
    Serial.printf("Config version: %d\n", config.version);
    if (config.version != CONFIG_VERSION) {
        Serial.println("Clearing Global Config");
        preferences.clear();
    }

    preferences.end();
}

void LoadGlobalConfig() {
    globalConfig.version = CONFIG_VERSION;
    globalConfig.brightness = 255;
    globalConfig.screenTimeout = 5;
    globalConfig.hotendPresets[0] = 0;
    globalConfig.hotendPresets[1] = 200;
    globalConfig.hotendPresets[2] = 240;
    globalConfig.bedPresets[0] = 0;
    globalConfig.bedPresets[1] = 60;
    globalConfig.bedPresets[2] = 70;
    VerifyVersion();
    Preferences preferences;
    preferences.begin(GLOBAL_CONFIG_NAMESPACE, true);
    preferences.getBytes(GLOBAL_CONFIG_KEY, &globalConfig, sizeof(globalConfig));
    preferences.end();
}
