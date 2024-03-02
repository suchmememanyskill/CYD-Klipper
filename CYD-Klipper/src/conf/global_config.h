#pragma once

#include "lvgl.h"

#define CONFIG_VERSION 4
#define TYPEC_CYD 1 // IMPORTANT! Type-C CYDs are inverted by default (apparently fixed in a experimental build, but this needs to have inverted to be on by default)

#define GLOBAL_CONFIG_NAMESPACE "global_config"
#define GLOBAL_CONFIG_KEY "global_config"

enum {
    REMAINING_TIME_CALC_PERCENTAGE = 0,
    REMAINING_TIME_CALC_INTERPOLATED = 1,
    REMAINING_TIME_CALC_SLICER = 2,
};

#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <stdexcept>

template <typename T>
class DynamicDropdownDataContainer {
public:
    virtual ~DynamicDropdownDataContainer() {}

    DynamicDropdownDataContainer(std::initializer_list<std::pair<std::string, T>> items) {
        addItems(items);
    }

    void addItems(std::initializer_list<std::pair<std::string, T>> items) {
        for (const auto& item : items) {
            _data.push_back(item);
        }
    }

    void removeItem(const T& value) {
        for (auto it = _data.begin(); it != _data.end(); ++it) {
            if (it->second == value) {
                _data.erase(it);
                break;
            }
        }
    }

    const char* getOptions() const {
        _optionsCache.clear();
        _optionsCache.reserve(_data.size() * (std::max(_data.begin()->first.size(), _data.rbegin()->first.size()) + 1));
        for (const auto& item : _data) {
            _optionsCache += item.first;
            _optionsCache += '\n';
        }
        return _optionsCache.c_str();
    }

    uint8_t getValues(T* values) const {
        uint8_t count = 0;
        for (const auto& item : _data) {
            values[count++] = item.second;
        }
        return count;
    }

    uint8_t getSelectedIndex(const T& value) const {
        uint8_t index = 0;
        for (const auto& item : _data) {
            if (item.second == value) {
                return index;
            }
            index++;
        }
        return 0;
    }

    std::pair<std::string, T> getByIndex(uint8_t index) const {
        if (index >= _data.size()) {
            throw std::out_of_range("Index out of range");
        }
        return _data[index];
    }

private:
    std::vector<std::pair<std::string, T>> _data;
    mutable std::string _optionsCache;
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
            bool darkMode : 1;
            #if !TYPEC_CYD 
            bool invertColors : 1;
            #endif
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
    short brightnessModulate;
    lv_palette_t secondaryColor;
} ColorDef;

extern GlobalConfig globalConfig;
extern ColorDef colorDefs[];

void WriteGlobalConfig();
void VerifyVersion();
void LoadGlobalConfig();
