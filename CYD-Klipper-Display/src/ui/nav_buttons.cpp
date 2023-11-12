#include "lvgl.h"
#include "panels/panel.h"
#include "../core/data_setup.h"
#include "nav_buttons.h"
#include <HTTPClient.h>

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

    if (hours > 99){
        lv_label_set_text(label, ">99h");
        return;
    }

    if (hours >= 1){
        sprintf(time_buffer, "%02luh%02lum", hours, minutes);
    } else {
        sprintf(time_buffer, "%02lum%02lus", minutes, seconds);
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

void nav_buttons_setup(unsigned char active_panel){
    lv_obj_clean(lv_scr_act());
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    sprintf(temp_buffer, "%.0f/%.0f", printer.extruder_temp, printer.bed_temp);
    sprintf(z_pos_buffer, "Z%.2f", printer.position[2]);

    const int button_width = 40;
    const int button_height = 60;
    const int icon_text_spacing = 10;

    // Files/Print
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, button_width, button_height);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_style(btn, &nav_button_style, 0);
    lv_obj_add_event_cb(btn, btn_click_files, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_COPY);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -1 * icon_text_spacing);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Idle");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, icon_text_spacing);
    lv_obj_add_style(label, &nav_button_text_style, 0);
    lv_obj_add_event_cb(label, update_printer_data_time, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_STATE, label, NULL);

    // Move
    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, button_width, button_height);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, button_height);
    lv_obj_add_style(btn, &nav_button_style, 0);
    lv_obj_add_event_cb(btn, btn_click_move, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CHARGE);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -1 * icon_text_spacing);
    
    label = lv_label_create(btn);
    lv_label_set_text(label, z_pos_buffer);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, icon_text_spacing);
    lv_obj_add_event_cb(label, update_printer_data_z_pos, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    lv_obj_add_style(label, &nav_button_text_style, 0);

    // Extrude/Temp
    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, button_width, button_height);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, button_height * 2);
    lv_obj_add_style(btn, &nav_button_style, 0);
    lv_obj_add_event_cb(btn, btn_click_extrude, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_WARNING);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -1 * icon_text_spacing);

    label = lv_label_create(btn);
    lv_label_set_text(label, temp_buffer);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, icon_text_spacing);
    lv_obj_add_event_cb(label, update_printer_data_temp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    lv_obj_add_style(label, &nav_button_text_style, 0);

    // Settings
    btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, button_width, button_height);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, button_height * 3);
    lv_obj_add_style(btn, &nav_button_style, 0);
    lv_obj_add_event_cb(btn, btn_click_settings, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -1 * icon_text_spacing);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Screen");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, icon_text_spacing);
    lv_obj_add_style(label, &nav_button_text_style, 0);

    lv_obj_t * panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(panel, TFT_HEIGHT - button_width, TFT_WIDTH);
    lv_obj_align(panel, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);

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
    }
}

void nav_style_setup(){
    lv_style_init(&nav_button_style);
    lv_style_set_radius(&nav_button_style, 0);

    lv_style_init(&nav_button_text_style);
    lv_style_set_text_font(&nav_button_text_style, &lv_font_montserrat_10);
}