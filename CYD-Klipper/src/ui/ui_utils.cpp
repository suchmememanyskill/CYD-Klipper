#include "lvgl.h"
#include "ui_utils.h"
#include "../core/data_setup.h"
#include "../core/lv_setup.h"
#include <ErriezCRC32.h>

lv_obj_t* lv_create_empty_panel(lv_obj_t* root) {
    lv_obj_t* panel = lv_obj_create(root); 
    lv_obj_set_style_border_width(panel, 0, 0); 
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    return panel;
}

void lv_layout_flex_column(lv_obj_t* obj, lv_flex_align_t allign, lv_coord_t pad_column, lv_coord_t pad_row){
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, allign, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(obj, pad_column, 0);
    lv_obj_set_style_pad_row(obj, pad_row, 0);
}

void lv_layout_flex_row(lv_obj_t* obj, lv_flex_align_t allign, lv_coord_t pad_column, lv_coord_t pad_row){
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, allign, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(obj, pad_column, 0);
    lv_obj_set_style_pad_row(obj, pad_row, 0);
}

void destroy_event_user_data(lv_event_t * e){
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_del(obj);
}

void lv_create_fullscreen_button_matrix_popup(lv_obj_t * root, lv_event_cb_t title, lv_button_column_t* columns, int column_count){
    const auto full_panel_width = CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 3;
    const auto full_panel_inner_width = full_panel_width - CYD_SCREEN_GAP_PX * 2 - 4;
    const auto full_panel_height = CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX;
    const auto full_panel_inner_height = full_panel_height - CYD_SCREEN_GAP_PX * 2 - 4;
    auto column_width = full_panel_inner_width / column_count - CYD_SCREEN_GAP_PX;
    auto column_height = full_panel_inner_height - CYD_SCREEN_GAP_PX - CYD_SCREEN_MIN_BUTTON_HEIGHT_PX;

    lv_obj_t * panel = lv_obj_create(root);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    lv_obj_set_size(panel, full_panel_width, full_panel_height);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * top_menu_row = lv_create_empty_panel(panel);
    lv_obj_set_size(top_menu_row, full_panel_inner_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(top_menu_row, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t * btn = lv_btn_create(top_menu_row);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, panel);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_center(label);

    label = lv_label_create(top_menu_row);
    lv_label_set_text(label, "-");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(label, title, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    
    lv_obj_t * rows = lv_create_empty_panel(panel);
    lv_obj_set_size(rows, full_panel_inner_width, column_height);
    lv_obj_align(rows, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_layout_flex_row(rows, LV_FLEX_ALIGN_SPACE_BETWEEN, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

    for (int i = 0; i < column_count; i++){
        lv_obj_t * column = lv_create_empty_panel(rows);
        lv_obj_clear_flag(column, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(column, column_width, column_height);
        lv_layout_flex_column(column, LV_FLEX_ALIGN_CENTER, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

        for (int j = 0; j < columns[i].length; j++){
            lv_obj_t * btn = lv_btn_create(column);
            lv_obj_set_size(btn, column_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
            lv_obj_add_event_cb(btn, columns[i].event, LV_EVENT_CLICKED, (void*)columns[i].data[j]);

            label = lv_label_create(btn);
            lv_label_set_text(label, columns[i].labels[j]);
            lv_obj_center(label);
        }
    }
}

void lv_keyboard_text_entry_close(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_DEFOCUSED || code == LV_EVENT_CANCEL || code == LV_EVENT_READY) 
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_del(lv_obj_get_parent(kb));
    }
}

void lv_create_keyboard_text_entry(lv_event_cb_t keyboard_callback, const char* title, lv_keyboard_mode_t keyboard_mode, lv_coord_t width, uint8_t max_length, const char* fill_text, bool contain_in_panel)
{
    lv_obj_t * parent = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_style_bg_opa(parent, LV_OPA_50, 0); 
    lv_obj_align(parent, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_layout_flex_column(parent, LV_FLEX_ALIGN_SPACE_BETWEEN);

    if (contain_in_panel)
    {
        lv_obj_set_size(parent, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    }
    else
    {
        lv_obj_set_size(parent, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    }

    if (title != nullptr)
    {
        lv_obj_t * empty_panel = lv_create_empty_panel(parent);
        lv_obj_set_size(empty_panel, 0, 0);

        lv_obj_t * title_container = lv_obj_create(parent);
        lv_obj_set_style_pad_all(title_container, CYD_SCREEN_GAP_PX / 2, 0);
        lv_obj_set_size(title_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        lv_obj_t * title_label = lv_label_create(title_container);
        lv_label_set_text(title_label, title);
    }

    lv_obj_t * empty_panel = lv_create_empty_panel(parent);
    lv_obj_set_flex_grow(empty_panel, 1);

    lv_obj_t * ta = lv_textarea_create(parent);
    lv_obj_t * keyboard = lv_keyboard_create(parent);

    lv_obj_set_width(ta, width);
    lv_textarea_set_max_length(ta, max_length);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, fill_text);
    lv_obj_add_event_cb(ta, keyboard_callback, LV_EVENT_READY, keyboard);
    lv_obj_add_event_cb(ta, lv_keyboard_text_entry_close, LV_EVENT_ALL, keyboard);

    lv_keyboard_set_mode(keyboard, keyboard_mode);
    lv_keyboard_set_textarea(keyboard, ta);
}

const static lv_point_t line_points[] = { {0, 0}, {(short int)((CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2) * 0.85f), 0} };

void lv_create_custom_menu_entry(const char* label_text, lv_obj_t* object, lv_obj_t* root_panel, bool set_height, const char * comment)
{
    lv_obj_t * panel = lv_create_empty_panel(root_panel);
    lv_layout_flex_row(panel, LV_FLEX_ALIGN_END);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, label_text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);

    lv_obj_set_parent(object, panel);

    if (set_height)
        lv_obj_set_height(object, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    if (comment != NULL)
    {
        lv_obj_t * comment_label = lv_label_create(root_panel);
        lv_label_set_text(comment_label, comment);
        lv_obj_set_style_text_font(comment_label, &CYD_SCREEN_FONT_SMALL, 0);
    }

    lv_obj_t * line = lv_line_create(root_panel);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
}

#define DROPDOWN_WIDTH CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 3.75
#define TOGGLE_WIDTH CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2

void lv_create_custom_menu_button(const char *label_text, lv_obj_t* root_panel, lv_event_cb_t on_click, const char *btn_text, void * user_data, const char * comment)
{
    lv_obj_t * btn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, user_data);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, btn_text);
    lv_obj_center(label);

    lv_create_custom_menu_entry(label_text, btn, root_panel, true, comment);
}

void lv_create_custom_menu_switch(const char *label_text, lv_obj_t* root_panel, lv_event_cb_t on_toggle, bool state, void * user_data, const char * comment)
{
    lv_obj_t * toggle = lv_switch_create(lv_scr_act());
    lv_obj_add_event_cb(toggle, on_toggle, LV_EVENT_VALUE_CHANGED, user_data);
    lv_obj_set_width(toggle, TOGGLE_WIDTH);

    if (state)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    lv_create_custom_menu_entry(label_text, toggle, root_panel, true, comment);
}

void lv_create_custom_menu_dropdown(const char *label_text, lv_obj_t *root_panel, lv_event_cb_t on_change, const char *options, int index, void * user_data, const char * comment)
{
    lv_obj_t * dropdown = lv_dropdown_create(lv_scr_act());
    lv_dropdown_set_options(dropdown, options);
    lv_dropdown_set_selected(dropdown, index);
    lv_obj_set_width(dropdown, DROPDOWN_WIDTH);
    lv_obj_add_event_cb(dropdown, on_change, LV_EVENT_VALUE_CHANGED, user_data);

    lv_create_custom_menu_entry(label_text, dropdown, root_panel, true, comment);
}

void lv_create_custom_menu_label(const char *label_text, lv_obj_t* root_panel, const char *text)
{
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, text);
    lv_create_custom_menu_entry(label_text, label, root_panel, false);
}

uint32_t message_hash = 0;
lv_timer_t* timer = NULL;

void on_timer_destroy(lv_event_t * e)
{
    lv_timer_del(timer);
    timer = NULL;
}

void timer_callback(lv_timer_t *timer)
{
    lv_obj_t * panel = (lv_obj_t *)timer->user_data;
    lv_obj_del(panel);
}

void lv_create_popup_message(const char* message, uint16_t timeout_ms)
{
    if (message == nullptr || timer != NULL) 
    {
        return;
    }

    uint32_t new_hash = crc32String(message);

    if (new_hash == message_hash) 
    {
        return;
    }

    message_hash = new_hash;

    lv_obj_t* panel = lv_obj_create(lv_scr_act()); 
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, LV_SIZE_CONTENT);
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(panel, LV_ALIGN_TOP_RIGHT, -CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);
    lv_obj_add_event_cb(panel, on_timer_destroy, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(panel, destroy_event_user_data, LV_EVENT_CLICKED, panel);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xFF0000), 0);

    lv_obj_t* label = lv_label_create(panel);
    lv_label_set_text_fmt(label, "%s", message);
    lv_obj_set_size(label, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 6, LV_SIZE_CONTENT);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    timer = lv_timer_create(timer_callback, timeout_ms,  panel);
}

lv_obj_t * lv_label_btn_create(lv_obj_t * parent, lv_event_cb_t btn_callback, void* user_data)
{
    lv_obj_t * panel = lv_create_empty_panel(parent);
    lv_obj_set_size(panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_add_event_cb(panel, btn_callback, LV_EVENT_CLICKED, user_data);

    lv_obj_t * label = lv_label_create(panel);
    return label;
}