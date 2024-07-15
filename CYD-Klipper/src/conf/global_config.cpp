#include <Preferences.h>
#include "global_config.h"
#include "lvgl.h"

GLOBAL_CONFIG global_config = {0};
TEMPORARY_CONFIG temporary_config = {0};

COLOR_DEF color_defs[] = {
    {LV_PALETTE_BLUE, 0, LV_PALETTE_RED},
    {LV_PALETTE_GREEN, 0, LV_PALETTE_PURPLE},
    {LV_PALETTE_LIME, -2, LV_PALETTE_PURPLE},
    {LV_PALETTE_GREY, 0, LV_PALETTE_CYAN},
    {LV_PALETTE_YELLOW, -2, LV_PALETTE_PINK},
    {LV_PALETTE_ORANGE, -2, LV_PALETTE_BLUE},
    {LV_PALETTE_RED, 0, LV_PALETTE_BLUE},
    {LV_PALETTE_PURPLE, 0, LV_PALETTE_CYAN},
};

void write_global_config()
{
    Preferences preferences;
    preferences.begin("global_config", false);
    preferences.putBytes("global_config", &global_config, sizeof(global_config));
    preferences.end();
}

void verify_version()
{
    Preferences preferences;
    if (!preferences.begin("global_config", false))
        return;
    
    GLOBAL_CONFIG config = {0};
    preferences.getBytes("global_config", &config, sizeof(config));
    LOG_F(("Config version: %d\n", config.version))
    if (config.version != CONFIG_VERSION) {
        LOG_LN("Clearing Global Config");
        preferences.clear();
    }

    preferences.end();
}

PRINTER_CONFIG* get_current_printer_config()
{
    return &global_config.printer_config[global_config.printer_index];
}

int get_printer_config_count()
{
    int count = 0;
    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++) {
        if (global_config.printer_config[i].ip_configured)
            count++;
    }
    return count;
}

int get_printer_config_free_index()
{
    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++) {
        if (!global_config.printer_config[i].ip_configured)
            return i;
    }
    return -1;
}

void set_printer_config_index(int index)
{
    if (index < 0 || index >= PRINTER_CONFIG_COUNT)
        return;

    PRINTER_CONFIG* old_config = &global_config.printer_config[global_config.printer_index];
    PRINTER_CONFIG* new_config = &global_config.printer_config[index];

    global_config.printer_index = index;

    if (!new_config->ip_configured){
        new_config->raw = old_config->raw;
        new_config->ip_configured = false;
        new_config->auth_configured = false;

        new_config->printer_name[0] = 0;
        new_config->klipper_host[0] = 0;
        new_config->klipper_auth[0] = 0;
        new_config->klipper_port = 0;

        new_config->color_scheme = old_config->color_scheme;

        // TODO: Replace with memcpy
        for (int i = 0; i < 3; i++){
            new_config->hotend_presets[i] = old_config->hotend_presets[i];
            new_config->bed_presets[i] = old_config->bed_presets[i];
        }

        for (int i = 0; i < 3; i++){
            new_config->printer_move_x_steps[i] = old_config->printer_move_x_steps[i];
            new_config->printer_move_y_steps[i] = old_config->printer_move_y_steps[i];
            new_config->printer_move_z_steps[i] = old_config->printer_move_z_steps[i];
        }

        write_global_config();
        ESP.restart();
    }    

    write_global_config();
}

void load_global_config() 
{
    global_config.version = CONFIG_VERSION;
    global_config.brightness = 255;
    global_config.screen_timeout = 5;
    global_config.printer_config[0].hotend_presets[0] = 0;
    global_config.printer_config[0].hotend_presets[1] = 200;
    global_config.printer_config[0].hotend_presets[2] = 240;
    global_config.printer_config[0].bed_presets[0] = 0;
    global_config.printer_config[0].bed_presets[1] = 60;
    global_config.printer_config[0].bed_presets[2] = 70;
    global_config.printer_config[0].printer_move_x_steps[0] = 10;
    global_config.printer_config[0].printer_move_x_steps[1] = 100;
    global_config.printer_config[0].printer_move_x_steps[2] = 1000;
    global_config.printer_config[0].printer_move_y_steps[0] = 10;
    global_config.printer_config[0].printer_move_y_steps[1] = 100;
    global_config.printer_config[0].printer_move_y_steps[2] = 1000;
    global_config.printer_config[0].printer_move_z_steps[0] = 1;
    global_config.printer_config[0].printer_move_z_steps[1] = 10;
    global_config.printer_config[0].printer_move_z_steps[2] = 100;

    verify_version();
    Preferences preferences;
    preferences.begin("global_config", true);
    preferences.getBytes("global_config", &global_config, sizeof(global_config));
    preferences.end();

    #if defined REPO_DEVELOPMENT  &&  REPO_DEVELOPMENT == 1
        temporary_config.debug = true;
    #else
        temporary_config.debug = false;
    #endif
    temporary_config.remote_echo = true;
}