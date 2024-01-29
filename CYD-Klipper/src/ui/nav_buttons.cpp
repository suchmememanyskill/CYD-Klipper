#include "lvgl.h"
#include "panels/panel.h"
#include "../core/data_setup.h"
#include "nav_buttons.h"
#include <HTTPClient.h>
#include "ui_utils.h"

static lv_style_t nav_button_style;

static char temp_buffer[10];
static char z_pos_buffer[10];
static char time_buffer[10];

static lv_style_t nav_button_text_style;

static void update_printer_data_z_pos(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    sprintf(z_pos_buffer, "Z%.2f", printer.position[2]);
    lv_label_set_text(label, z_pos_buffer);
}

static void update_printer_data_temp(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);

    sprintf(temp_buffer, "%.0f/%.0f", printer.extruder_temp, printer.bed_temp);
    lv_label_set_text(label, temp_buffer);
}

static void update_printer_data_time(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);

    if (printer.state == PRINTER_STATE_IDLE){
        lv_label_set_text(label, "Idle");
        return;
    }

    if (printer.state == PRINTER_STATE_PAUSED){
        lv_label_set_text(label, "Paused");
        return;
    }

    unsigned long time = printer.remaining_time_s;
    unsigned long hours = time / 3600;
    unsigned long minutes = (time % 3600) / 60;
    unsigned long seconds = (time % 3600) % 60;

    if (hours >= 10){
        sprintf(time_buffer, "%luh", hours);
    } else if (hours >= 1){
        sprintf(time_buffer, "%luh%02lum", hours, minutes);
    } else {
        sprintf(time_buffer, "%lum", minutes);
    }

    lv_label_set_text(label, time_buffer);
}

static void btn_click_files(lv_event_t * e){
    nav_buttons_setup(0);
}

static void btn_click_move(lv_event_t * e){
    nav_buttons_setup(1);
}

static void btn_click_extrude(lv_event_t * e){
    nav_buttons_setup(2);
}

static void btn_click_settings(lv_event_t * e){
    nav_buttons_setup(3);
}

static void btn_click_macros(lv_event_t * e){
    nav_buttons_setup(4);
}

void create_button(const char* icon, const char* name, lv_event_cb_t button_click, lv_event_cb_t label_update, lv_obj_t * root){
    lv_obj_t* btn = lv_btn_create(root);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_width(btn, CYD_SCREEN_SIDEBAR_SIZE_PX);
    lv_obj_add_style(btn, &nav_button_style, 0);
    if (button_click != NULL)
        lv_obj_add_event_cb(btn, button_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, icon);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -1 * CYD_SCREEN_GAP_PX);
    
    label = lv_label_create(btn);
    lv_label_set_text(label, name);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, CYD_SCREEN_GAP_PX);
    lv_obj_add_event_cb(label, label_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    lv_obj_add_style(label, &nav_button_text_style, 0);
}

void nav_buttons_setup(unsigned char active_panel){
    lv_obj_clean(lv_scr_act());
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * root_panel = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(root_panel, CYD_SCREEN_SIDEBAR_SIZE_PX, CYD_SCREEN_HEIGHT_PX); 
    lv_obj_align(root_panel, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_layout_flex_column(root_panel, LV_FLEX_ALIGN_START, 0, 0);

    // Files/Print
    create_button(LV_SYMBOL_COPY, "Idle", btn_click_files, update_printer_data_time, root_panel);

    // Move
    create_button(LV_SYMBOL_CHARGE, "Z?", btn_click_move, update_printer_data_z_pos, root_panel);

    // Extrude/Temp
    create_button(LV_SYMBOL_WARNING, "?/?", btn_click_extrude, update_printer_data_temp, root_panel);

    // Macros
    create_button(LV_SYMBOL_GPS, "Macro", btn_click_macros, NULL, root_panel);

    lv_obj_t * panel = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_align(panel, LV_ALIGN_TOP_RIGHT, 0, 0);

    switch (active_panel){
        case 0:
            print_panel_init(panel);
            break;
        case 1:
            move_panel_init(panel);
            break;
        case 2:
            temp_panel_init(panel);
            break;
        case 3:
            settings_panel_init(panel);
            break;
        case 4:
            macros_panel_init(panel);
            break;
    }

    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

void nav_style_setup(){
    lv_style_init(&nav_button_style);
    lv_style_set_radius(&nav_button_style, 0);

    lv_style_init(&nav_button_text_style);
    lv_style_set_text_font(&nav_button_text_style, CYD_SCREEN_FONT_SMALL);
}