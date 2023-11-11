#include <Preferences.h>
#include "global_config.h"

GLOBAL_CONFIG global_config = {0};

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
    VerifyVersion();
    Preferences preferences;
    preferences.begin("global_config", true);
    preferences.getBytes("global_config", &global_config, sizeof(global_config));
    preferences.end();

    Serial.printf("Touch: %d\n", global_config.screenCalibrated);
}