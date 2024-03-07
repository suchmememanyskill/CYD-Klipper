#include "panel.h"
#include "../../conf/global_config.h"
#include "../../core/data_setup.h"
#include "../ui_utils.h"

const char * printer_status[] = {
    "Error",
    "Idle",
    "Printing",
    "Paused"
};

const static lv_point_t line_points[] = { {0, 0}, {(short int)((CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2) * 0.85f), 0} };

static void update_printer_status_text(lv_event_t * e) 
{
    lv_obj_t * label = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if (!printer->online){
        lv_label_set_text(label, "Offline");
        return;
    }

    lv_label_set_text(label, printer_status[printer->state]);
}

static void update_printer_percentage_bar(lv_event_t * e)
{
    lv_obj_t * percentage = lv_event_get_target(e);
    PRINTER_CONFIG * config = (PRINTER_CONFIG*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;
    PrinterMinimal * printer = &printer_minimal[index];

    if (printer->online && (printer->state == PRINTER_STATE_PRINTING || printer->state == PRINTER_STATE_PAUSED)){
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

    if (printer->online && (printer->state == PRINTER_STATE_PRINTING || printer->state == PRINTER_STATE_PAUSED)){
        lv_label_set_text_fmt(label, "%.2f%%", printer->print_progress * 100);
    }
    else {
        lv_label_set_text(label, "-");
    }
}

static void btn_printer_delete(lv_event_t * e)
{

}

static void btn_printer_rename(lv_event_t * e)
{

}

static void btn_printer_activate(lv_event_t * e)
{

}

void create_printer_ui(PRINTER_CONFIG * config, lv_obj_t * root)
{
    int index = config - global_config.printer_config;
    auto width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * data_row_name = lv_create_empty_panel(root);
    lv_layout_flex_row(data_row_name, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_size(data_row_name, width, LV_SIZE_CONTENT);

    lv_obj_t * label = lv_label_create(data_row_name);
    lv_label_set_text(label, config->printer_name[0] == 0 ? config->klipper_host : config->printer_name);
    
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
    lv_obj_add_event_cb(btn, btn_printer_delete, LV_EVENT_CLICKED, config);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_TRASH);
    lv_obj_center(label);

    btn = lv_btn_create(button_row);
    lv_obj_set_flex_grow(btn, 2);
    lv_obj_add_event_cb(btn, btn_printer_rename, LV_EVENT_CLICKED, config);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Rename");
    lv_obj_center(label);

    btn = lv_btn_create(button_row);
    lv_obj_set_flex_grow(btn, 2);
    lv_obj_add_event_cb(btn, btn_printer_activate, LV_EVENT_CLICKED, config);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Activate");
    lv_obj_center(label);

    lv_obj_t * line = lv_line_create(root);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
}

void printer_panel_init(lv_obj_t* panel) 
{
    lv_obj_t * inner_panel = lv_create_empty_panel(panel);
    lv_obj_align(inner_panel, LV_ALIGN_TOP_LEFT, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);
    lv_obj_set_size(inner_panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_PANEL_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_column(inner_panel);
    lv_obj_set_scrollbar_mode(inner_panel, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++){
        PRINTER_CONFIG * config = &global_config.printer_config[i];
        if (config->ip_configured) {
            create_printer_ui(&global_config.printer_config[i], inner_panel);
        }
    }

    lv_msg_send(DATA_PRINTER_MINIMAL, NULL);
}