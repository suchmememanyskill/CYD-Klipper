#include "switch_printer.h"
#include "lvgl.h"
#include "../conf/global_config.h"
#include "ui_utils.h"
#include "../core/http_client.h"
#include "../core/lv_setup.h"
#include "../core/macros_query.h"

static void btn_switch_printer(lv_event_t *e){
    lv_obj_t *btn = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;

    set_printer_config_index(index);
    set_color_scheme();
    _macros_query_internal();
    _power_devices_query_internal();

    lv_obj_del(lv_obj_get_parent(lv_obj_get_parent(btn)));
}

void switch_printer_init() {
    lv_obj_t * parent = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_style_bg_opa(parent, LV_OPA_100, 0); 
    lv_obj_align(parent, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(parent, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(parent);

    lv_obj_set_size(lv_create_empty_panel(parent), 0, 0);

    auto width = CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, parent);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Close");
    lv_obj_center(label);

    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++){
        PRINTER_CONFIG * config = &global_config.printer_config[i];
        const char* printer_name = (config->printer_name[0] == 0) ? config->klipper_host : config->printer_name;

        if (config == get_current_printer_config())
        {
            lv_create_custom_menu_label(printer_name, parent, "Active");
            continue;
        }

        if (config->ip_configured) {
            HTTPClient client;
            configure_http_client(client, get_full_url("/printer/objects/query?webhooks&print_stats&virtual_sdcard", config), true, 1000);
            

            int httpCode = client.GET();
            if (httpCode == 200)
            {
                lv_create_custom_menu_button(printer_name, parent, btn_switch_printer, "Switch", config);
            }
            else 
            {
                lv_create_custom_menu_label(printer_name, parent, "Offline");
            }
        }
    }
}

static void show_switch_printer_screen(lv_event_t * e){
    switch_printer_init();
}

void draw_switch_printer_button() 
{
    if (!global_config.multi_printer_mode)
    {
        return;
    }

    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);
    lv_obj_add_event_cb(btn, show_switch_printer_screen, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME);
    lv_obj_center(label);
}