#include "panel.h"
#include "../../conf/global_config.h"
#include "../../core/data_setup.h"
#include "../ui_utils.h"
#include "../../core/lv_setup.h"
#include <stdio.h>
#include "../nav_buttons.h"
#include "../../core/macros_query.h"
#include "../switch_printer.h"
#include "../macros.h"

const char * printer_status[] = {
    "Offline",
    "Error",
    "Idle",
    "Printing",
    "Paused"
};

const static lv_point_t line_points[] = { {0, 0}, {(short int)((CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2) * 0.85f), 0} };

static void update_printer_name_text(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    lv_label_set_text(label, config->printer_name[0] == 0 ? config->klipper_host : config->printer_name);
}

static void update_printer_status_text(lv_event_t * e) 
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if (config == get_current_printer_config())
    {
        lv_label_set_text(label, "In Control");
        return;
    }

    if (printer->state == PRINTER_STATE_OFFLINE)
    {
        lv_label_set_text(label, "Offline");
        return;
    }

    lv_label_set_text(label, printer_status[printer->state]);
}

static void update_printer_label_visible_active_printer(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);

    if (config == get_current_printer_config())
    {
        lv_label_set_text(label, LV_SYMBOL_WIFI);
    }
    else 
    {
        lv_label_set_text(label, "");
    }
}

static void update_printer_percentage_bar(lv_event_t * e)
{
    lv_obj_t * percentage = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if (printer->state != PRINTER_STATE_OFFLINE && (printer->state == PRINTER_STATE_PRINTING || printer->state == PRINTER_STATE_PAUSED)){
        lv_bar_set_value(percentage, printer->print_progress * 100, LV_ANIM_OFF);
    }
    else {
        lv_bar_set_value(percentage, 0, LV_ANIM_OFF);
    }
}

static void update_printer_percentage_text(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if (printer->state != PRINTER_STATE_OFFLINE && (printer->state == PRINTER_STATE_PRINTING || printer->state == PRINTER_STATE_PAUSED))
    {
        char percentage_buffer[12];
        sprintf(percentage_buffer, "%.2f%%", printer->print_progress * 100);
        lv_label_set_text(label, percentage_buffer);
    }
    else 
    {
        lv_label_set_text(label, "-%");
    }
}

static void update_printer_control_button_text(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if (printer->power_devices > 0 && (config == get_current_printer_config() || printer->state == PRINTER_STATE_OFFLINE))
    {
        lv_label_set_text(label, "Power");
    }
    else
    {
        lv_label_set_text(label, "Control");
    }
}

static void btn_set_secondary_button_text(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);

    if (config == get_current_printer_config())
    {
        lv_label_set_text(label, LV_SYMBOL_SETTINGS);
    }
    else 
    {
        lv_label_set_text(label, LV_SYMBOL_TRASH);
    }
}

static void btn_enable_control(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if ((config == get_current_printer_config() || printer->state == PRINTER_STATE_OFFLINE) && printer->power_devices <= 0)
    {
        // Disable
        lv_obj_add_state(btn, LV_STATE_DISABLED);

    }
    else
    {
        // Enable
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
    }
}

PRINTER_CONFIG * keyboard_config = NULL;

static void keyboard_callback(lv_event_t * e){
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);
    
    const char * text = lv_textarea_get_text(ta);
    strcpy(keyboard_config->printer_name, text);
    write_global_config();
    lv_msg_send(DATA_PRINTER_MINIMAL, NULL);
}

static void btn_printer_secondary(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);

    if (config == get_current_printer_config())
    {
        nav_buttons_setup(PANEL_SETTINGS);
        return;
    }

    config->ip_configured = false;
    write_global_config();

    nav_buttons_setup(PANEL_PRINTER);
}

static void btn_printer_rename(lv_event_t * e)
{
    keyboard_config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    lv_create_keyboard_text_entry(keyboard_callback, LV_KEYBOARD_MODE_TEXT_LOWER, CYD_SCREEN_WIDTH_PX * 0.75, 24, keyboard_config->printer_name, false);
}

static void btn_printer_activate(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if (printer->power_devices > 0 && (config == get_current_printer_config() || printer->state == PRINTER_STATE_OFFLINE))
    {
        macros_draw_power_fullscreen(config);
        return;
    }

    switch_printer(index);
    lv_msg_send(DATA_PRINTER_MINIMAL, NULL);
}

static void btn_printer_add(lv_event_t * e)
{
    set_printer_config_index(get_printer_config_free_index());
}

void create_printer_ui(PRINTER_CONFIG * config, lv_obj_t * root)
{
    int index = config - global_config.printer_config;
    auto width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * data_row_name = lv_create_empty_panel(root);
    lv_layout_flex_row(data_row_name, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_size(data_row_name, width, LV_SIZE_CONTENT);

    lv_obj_t * label = lv_label_create(data_row_name);
    lv_obj_add_event_cb(label, update_printer_name_text, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, label, config);

    label = lv_label_create(data_row_name);
    lv_obj_add_event_cb(label, update_printer_label_visible_active_printer, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, label, config);
    
    label = lv_label_create(data_row_name);
    lv_obj_add_event_cb(label, update_printer_status_text, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, label, config);

    lv_obj_t * progress_row = lv_create_empty_panel(root);
    lv_layout_flex_row(progress_row);
    lv_obj_set_size(progress_row, width, LV_SIZE_CONTENT);
    
    lv_obj_t * progress_bar = lv_bar_create(progress_row);
    lv_obj_set_flex_grow(progress_bar, 1);
    lv_obj_add_event_cb(progress_bar, update_printer_percentage_bar, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, progress_bar, config);

    label = lv_label_create(progress_row);
    lv_obj_set_style_text_font(label, &CYD_SCREEN_FONT_SMALL, 0);
    lv_obj_add_event_cb(label, update_printer_percentage_text, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, label, config);

    lv_obj_t * button_row = lv_create_empty_panel(root);
    lv_layout_flex_row(button_row);
    lv_obj_set_size(button_row, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * btn = lv_btn_create(button_row);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, btn_printer_secondary, LV_EVENT_CLICKED, config);

    label = lv_label_create(btn);
    lv_obj_center(label);
    lv_obj_add_event_cb(label, btn_set_secondary_button_text, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, label, config);

    btn = lv_btn_create(button_row);
    lv_obj_set_flex_grow(btn, 2);
    lv_obj_add_event_cb(btn, btn_printer_rename, LV_EVENT_CLICKED, config);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Rename");
    lv_obj_center(label);

    btn = lv_btn_create(button_row);
    lv_obj_set_flex_grow(btn, 2);
    lv_obj_add_event_cb(btn, btn_printer_activate, LV_EVENT_CLICKED, config);
    lv_obj_add_event_cb(btn, btn_enable_control, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, btn, config);

    label = lv_label_create(btn);
    lv_obj_center(label);
    lv_obj_add_event_cb(label, update_printer_control_button_text, LV_EVENT_MSG_RECEIVED, config);
    lv_msg_subsribe_obj(DATA_PRINTER_MINIMAL, label, config);

    lv_obj_t * line = lv_line_create(root);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
}

void printer_panel_init(lv_obj_t* panel) 
{
    lv_obj_t * inner_panel = lv_create_empty_panel(panel);
    lv_obj_align(inner_panel, LV_ALIGN_TOP_LEFT, CYD_SCREEN_GAP_PX, 0);
    lv_obj_set_size(inner_panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_layout_flex_column(inner_panel);
    lv_obj_set_scrollbar_mode(inner_panel, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_size(lv_create_empty_panel(inner_panel), 0, 0);

    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++){
        PRINTER_CONFIG * config = &global_config.printer_config[i];
        if (config->ip_configured) {
            create_printer_ui(&global_config.printer_config[i], inner_panel);
        }
    }

    // Add Printer Button
    if (get_printer_config_free_index() != -1){
        lv_obj_t * btn = lv_btn_create(inner_panel);
        lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_add_event_cb(btn, btn_printer_add, LV_EVENT_CLICKED, NULL);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, "Add Printer");
        lv_obj_center(label);
    }

    lv_obj_set_size(lv_create_empty_panel(inner_panel), 0, 0);

    lv_msg_send(DATA_PRINTER_MINIMAL, NULL);
}