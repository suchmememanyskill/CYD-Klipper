#ifndef _GLOBAL_CONFIG_INIT
#define _GLOBAL_CONFIG_INIT

#define CONFIG_VERSION 80

typedef struct _GLOBAL_CONFIG {
    unsigned char version;
    union {
        unsigned char raw;
        struct {
            bool screenCalibrated : 1;
            bool wifiConfigured : 1;
            bool ipConfigured : 1;
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
} GLOBAL_CONFIG;

extern GLOBAL_CONFIG global_config;

void WriteGlobalConfig();
void VerifyVersion();
void LoadGlobalConfig();

#endif // !_GLOBAL_CONFIG_INIT