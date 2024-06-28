#include "lvgl.h"
#include "panels/panel.h"
#include "../core/data_setup.h"
#include "nav_buttons.h"
#include "ui_utils.h"
#include <stdio.h>
#include "../conf/global_config.h"

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
    nav_buttons_setup(PANEL_FILES);
}

static void btn_click_progress(lv_event_t * e){
    nav_buttons_setup(PANEL_PROGRESS);
}

static void btn_click_move(lv_event_t * e){
    nav_buttons_setup(PANEL_MOVE);
}

static void btn_click_extrude(lv_event_t * e){
    nav_buttons_setup(PANEL_TEMP);
}

static void btn_click_settings(lv_event_t * e){
    nav_buttons_setup(PANEL_SETTINGS);
}

static void btn_click_macros(lv_event_t * e){
    nav_buttons_setup(PANEL_MACROS);
}

static void btn_click_printer(lv_event_t * e){
    nav_buttons_setup(PANEL_PRINTER);
}

static void btn_click_err(lv_event_t * e){
    nav_buttons_setup(PANEL_ERROR);
}

static void btn_click_conn(lv_event_t * e){
    nav_buttons_setup(PANEL_CONNECTING);
}

void create_button(const char* icon, const char* name, lv_event_cb_t button_click, lv_event_cb_t label_update, lv_obj_t * root){
    lv_obj_t* btn = lv_btn_create(root);
    lv_obj_set_flex_grow(btn, 1);

#ifdef CYD_SCREEN_VERTICAL
    lv_obj_set_height(btn, CYD_SCREEN_SIDEBAR_SIZE_PX);
#else
    lv_obj_set_width(btn, CYD_SCREEN_SIDEBAR_SIZE_PX);
#endif

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

void nav_buttons_setup(PANEL_TYPE active_panel){
    lv_obj_clean(lv_scr_act());
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * root_panel = lv_create_empty_panel(lv_scr_act());

#ifdef CYD_SCREEN_VERTICAL
    lv_obj_set_size(root_panel, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_SIDEBAR_SIZE_PX); 
    lv_obj_align(root_panel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_layout_flex_row(root_panel, LV_FLEX_ALIGN_START, 0, 0);
#else
    lv_obj_set_size(root_panel, CYD_SCREEN_SIDEBAR_SIZE_PX, CYD_SCREEN_HEIGHT_PX); 
    lv_obj_align(root_panel, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_layout_flex_column(root_panel, LV_FLEX_ALIGN_START, 0, 0);

#endif

    if (printer.state > PRINTER_STATE_ERROR){
        // Files/Print
        if (printer.state == PRINTER_STATE_IDLE)
        {
            create_button(LV_SYMBOL_COPY, "Idle", btn_click_files, update_printer_data_time, root_panel);
        }
        else 
        {
            create_button(LV_SYMBOL_FILE, "Paused", btn_click_progress, update_printer_data_time, root_panel);
        }

        // Move
        create_button(printer.state == PRINTER_STATE_PRINTING ? LV_SYMBOL_EDIT : LV_SYMBOL_CHARGE, "Z?", btn_click_move, update_printer_data_z_pos, root_panel);

        // Extrude/Temp
        create_button(LV_SYMBOL_WARNING, "?/?", btn_click_extrude, update_printer_data_temp, root_panel);
    }
    else if (printer.state == PRINTER_STATE_ERROR) {
        // Error UI
        create_button(LV_SYMBOL_WARNING, "Error", btn_click_err, NULL, root_panel);
    }
    else {
        // Connecting
        create_button(LV_SYMBOL_REFRESH, "Link", btn_click_conn, NULL, root_panel);
    }

    // Macros
    create_button(LV_SYMBOL_GPS, "Macro", btn_click_macros, NULL, root_panel);

    if (global_config.multi_printer_mode)
    {
        // Printers
        create_button(LV_SYMBOL_HOME, "Printer", btn_click_printer, NULL, root_panel);
    }

    lv_obj_t * panel = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_align(panel, LV_ALIGN_TOP_RIGHT, 0, 0);

    switch (active_panel){
        case PANEL_FILES:
            files_panel_init(panel);
            break;
        case PANEL_MOVE:
            move_panel_init(panel);
            break;
        case PANEL_TEMP:
            temp_panel_init(panel);
            break;
        case PANEL_SETTINGS:
            settings_panel_init(panel);
            break;
        case PANEL_MACROS:
            macros_panel_init(panel);
            break;
        case PANEL_STATS:
            stats_panel_init(panel);
            break;
        case PANEL_PRINTER:
            printer_panel_init(panel);
            break;
        case PANEL_ERROR:
            error_panel_init(panel);
            break;
        case PANEL_CONNECTING:
            connecting_panel_init(panel);
            break;
        case PANEL_PROGRESS:
            progress_panel_init(panel);
            break;
    }

    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

void nav_style_setup(){
    lv_style_init(&nav_button_style);
    lv_style_set_radius(&nav_button_style, 0);

    lv_style_init(&nav_button_text_style);
    lv_style_set_text_font(&nav_button_text_style, &CYD_SCREEN_FONT_SMALL);
}