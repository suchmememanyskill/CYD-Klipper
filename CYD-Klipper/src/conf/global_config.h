#ifndef _GLOBAL_CONFIG_INIT
#define _GLOBAL_CONFIG_INIT

#include "lvgl.h"

#define CONFIG_VERSION 4

enum {
    REMAINING_TIME_CALC_PERCENTAGE = 0,
    REMAINING_TIME_CALC_INTERPOLATED = 1,
    REMAINING_TIME_CALC_SLICER = 2,
};

typedef struct _GLOBAL_CONFIG {
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
            unsigned char remaining_time_calc_mode : 2;

            // Internal
            bool auth_configured : 1;
        };
    };
    float screenCalXOffset;
    float screenCalXMult;
    float screenCalYOffset;
    float screenCalYMult;

    char wifiSSID[32];
    char wifiPassword[64];

    char klipperHost[64];
    unsigned short klipperPort;
    
    unsigned char color_scheme;
    unsigned char brightness;
    unsigned char screenTimeout;

    unsigned short hotend_presets[3];
    unsigned short bed_presets[3];

    char klipper_auth[33];
} GLOBAL_CONFIG;
    
typedef struct _COLOR_DEF {
    lv_palette_t primary_color;
    short primary_color_light;
    lv_palette_t secondary_color;
} COLOR_DEF;

extern GLOBAL_CONFIG global_config;
extern COLOR_DEF color_defs[];

void WriteGlobalConfig();
void VerifyVersion();
void LoadGlobalConfig();

#endif // !_GLOBAL_CONFIG_INIT