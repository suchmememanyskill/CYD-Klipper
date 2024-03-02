#pragma once

#include "lvgl.h"

#define CONFIG_VERSION 4
#define GLOBAL_CONFIG_NAMESPACE "global_config"
#define GLOBAL_CONFIG_KEY "global_config"

enum {
    REMAINING_TIME_CALC_PERCENTAGE = 0,
    REMAINING_TIME_CALC_INTERPOLATED = 1,
    REMAINING_TIME_CALC_SLICER = 2,
};

typedef struct {
    unsigned char version;
    union {
        unsigned int raw;
        struct {
            // Internal
            bool screenCalibrated : 1;
            bool wifiConfigured : 1;
            bool ipConfigured : 1;

            // External
            bool lightMode : 1;
            bool invertColors : 1;
            bool rotateScreen : 1;
            bool onDuringPrint : 1;
            bool autoOtaUpdate : 1;
            unsigned char remainingTimeCalcMode : 2;

            // Internal
            bool authConfigured : 1;
        };
    };
    float screenCalXOffset;
    float screenCalXMult;
    float screenCalYOffset;
    float screenCalYMult;

    char wifiSsid[32];
    char wifiPassword[64];

    char klipperHost[64];
    unsigned short klipperPort;

    unsigned char colorScheme;
    unsigned char brightness;
    unsigned char screenTimeout;

    unsigned short hotendPresets[3];
    unsigned short bedPresets[3];

    char klipperAuth[33];
} GlobalConfig;

typedef struct {
    lv_palette_t primaryColor;
    short primaryColorLight;
    lv_palette_t secondaryColor;
} ColorDef;

extern GlobalConfig globalConfig;
extern ColorDef colorDefs[];

void WriteGlobalConfig();
void VerifyVersion();
void LoadGlobalConfig();
