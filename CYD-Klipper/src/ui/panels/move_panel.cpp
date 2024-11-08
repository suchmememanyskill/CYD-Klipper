#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"
#include "../nav_buttons.h"
#include "../ui_utils.h"
#include <stdio.h>
#include <Esp.h>
#include "../../core/printer_integration.hpp"

static bool last_homing_state = false;
static bool move_edit_mode = false;

float x_offsets[6] = {0};
float y_offsets[6] = {0};
float z_offsets[6] = {0};

#define OFFSET_LABEL_SIZE 7

char x_offset_labels[6 * OFFSET_LABEL_SIZE] = {0};
char y_offset_labels[6 * OFFSET_LABEL_SIZE] = {0};
char z_offset_labels[6 * OFFSET_LABEL_SIZE] = {0};

static void calculate_offsets_from_current_printer()
{
    unsigned short* items[] = {get_current_printer()->printer_config->printer_move_x_steps, get_current_printer()->printer_config->printer_move_y_steps, get_current_printer()->printer_config->printer_move_z_steps};
    float* offsets[] = {(float*)x_offsets, (float*)y_offsets, (float*)z_offsets};
    char * labels[] = {(char*)x_offset_labels, (char*)y_offset_labels, (char*)z_offset_labels};

    for (int i = 0; i < 3; i++)
    {
        offsets[i][0] = items[i][2] / 10.0f * -1;
        offsets[i][1] = items[i][1] / 10.0f * -1;
        offsets[i][2] = items[i][0] / 10.0f * -1;
        offsets[i][3] = items[i][0] / 10.0f;
        offsets[i][4] = items[i][1] / 10.0f;
        offsets[i][5] = items[i][2] / 10.0f;

        for (int j = 0; j < 6; j++) {
            const char * formats[] = {"%.0f", "%.1f", "+%.0f", "+%.1f"};
            const char ** format = formats;

            if (offsets[i][j] != (int)offsets[i][j])
            {
                format += 1;
            }

            if (j >= 3)
            {
                format += 2;
            }

            sprintf(labels[i] + OFFSET_LABEL_SIZE * j, *format, offsets[i][j]);
        }
    }
}

static int selected_column = 0;
static int selected_row = 0;

static void keyboard_cb_edit_move_increment(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);
    const char * text = lv_textarea_get_text(ta);

    float increment = atof(text);

    if (increment < 0)
    {
        increment *= -1;
    }

    if (increment == 0 || increment > 999)
    {
        return;
    }
    
    unsigned short* items[] = {get_current_printer()->printer_config->printer_move_x_steps, get_current_printer()->printer_config->printer_move_y_steps, get_current_printer()->printer_config->printer_move_z_steps};
    LOG_F(("Setting increment %d %d %f\n", selected_column, selected_row, increment))
    items[selected_column][selected_row] = increment * 10;
    write_global_config();
    nav_buttons_setup(PANEL_MOVE);
}

static void edit_move_increment(int column, float* idx)
{
    float* offsets[] = {(float*)x_offsets, (float*)y_offsets, (float*)z_offsets};
    int row = idx - offsets[column];

    if (row < 3)
    {
        selected_row = 2 - row;
    }
    else 
    {
        selected_row = row - 3;
    }

    selected_column = column;
    lv_create_keyboard_text_entry(keyboard_cb_edit_move_increment, "Set increment", LV_KEYBOARD_MODE_NUMBER, LV_PCT(75), 6);
}

static void x_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);

    if (move_edit_mode)
    {
        edit_move_increment(0, data_pointer);
        return;
    }
    
    float data = *data_pointer;
    get_current_printer()->move_printer("X", data, true);
}

static void y_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);

    if (move_edit_mode)
    {
        edit_move_increment(1, data_pointer);
        return;
    }

    float data = *data_pointer;
    get_current_printer()->move_printer("Y", data, true);
}

static void z_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);

    if (move_edit_mode)
    {
        edit_move_increment(2, data_pointer);
        return;
    }

    float data = *data_pointer;
    get_current_printer()->move_printer("Z", data, true);
}

static void x_pos_update(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char x_pos_buff[12];
    sprintf(x_pos_buff, "X: %.1f", get_current_printer_data()->position[0]);
    lv_label_set_text(label, x_pos_buff);
}

static void y_pos_update(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char y_pos_buff[12];
    sprintf(y_pos_buff, "Y: %.1f", get_current_printer_data()->position[1]);
    lv_label_set_text(label, y_pos_buff);
}

static void z_pos_update(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char z_pos_buff[12];
    sprintf(z_pos_buff, "Z: %.2f", get_current_printer_data()->position[2]);
    lv_label_set_text(label, z_pos_buff);
}

lv_event_cb_t button_callbacks[] = {x_line_button_press, y_line_button_press, z_line_button_press};
lv_event_cb_t position_callbacks[] = {x_pos_update, y_pos_update, z_pos_update};

static void home_button_click(lv_event_t * e) {
    if (get_current_printer_data()->state == PrinterState::PrinterStatePrinting)
        return;

    freeze_request_thread();
    get_current_printer()->execute_feature(PrinterFeatures::PrinterFeatureHome);
    unfreeze_request_thread();
} 

static void disable_steppers_click(lv_event_t * e) {
    if (get_current_printer_data()->state == PrinterState::PrinterStatePrinting)
        return;

    get_current_printer()->execute_feature(PrinterFeatures::PrinterFeatureDisableSteppers);
} 

static void switch_to_stat_panel(lv_event_t * e) {
    lv_obj_t * panel = lv_event_get_target(e);
    nav_buttons_setup(PANEL_STATS);
}

static void move_edit_toggle(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    move_edit_mode = lv_obj_get_state(btn) & LV_STATE_CHECKED;
}

static void line_custom_set(const char * axis, const char *text)
{
    float pos = atof(text);

    if (pos < 0 || pos > 500)
        return;

    get_current_printer()->move_printer(axis, pos, false);
}

static void x_line_custom_callback(lv_event_t * e) {
    const char * text = lv_textarea_get_text(lv_event_get_target(e));
    line_custom_set("X", text);
}

static void y_line_custom_callback(lv_event_t * e) {
    const char * text = lv_textarea_get_text(lv_event_get_target(e));
    line_custom_set("Y", text);
}

static void z_line_custom_callback(lv_event_t * e) {
    const char * text = lv_textarea_get_text(lv_event_get_target(e));
    line_custom_set("Z", text);
}

static void x_line_custom(lv_event_t * e) {
    lv_create_keyboard_text_entry(x_line_custom_callback, "Set X position", LV_KEYBOARD_MODE_NUMBER, LV_PCT(75), 6);
}

static void y_line_custom(lv_event_t * e) {
    lv_create_keyboard_text_entry(y_line_custom_callback, "Set Y position", LV_KEYBOARD_MODE_NUMBER, LV_PCT(75), 6);
}

static void z_line_custom(lv_event_t * e) {
    lv_create_keyboard_text_entry(z_line_custom_callback, "Set Z position", LV_KEYBOARD_MODE_NUMBER, LV_PCT(75), 6);
}

lv_event_cb_t custom_callbacks[] = {x_line_custom, y_line_custom, z_line_custom};

inline void root_panel_steppers_locked(lv_obj_t * root_panel){
    const auto width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * panel = lv_create_empty_panel(root_panel);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_SPACE_BETWEEN, 0, 0);

    lv_obj_t * home_button_row = lv_create_empty_panel(panel);
    lv_obj_set_size(home_button_row, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(home_button_row);

    lv_obj_t * btn = lv_btn_create(home_button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, home_button_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME "Home");
    lv_obj_center(label);

    btn = lv_btn_create(home_button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, disable_steppers_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_EYE_CLOSE "Free");
    lv_obj_center(label);

    btn = lv_btn_create(home_button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, switch_to_stat_panel, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS "Param");
    lv_obj_center(label);

    btn = lv_btn_create(home_button_row);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, move_edit_toggle, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

    if (move_edit_mode)
    {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    }

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_EDIT);
    lv_obj_center(label);

    float* offsets[] = {(float*)x_offsets, (float*)y_offsets, (float*)z_offsets};
    char * labels[] = {(char*)x_offset_labels, (char*)y_offset_labels, (char*)z_offset_labels};

    for (int row = 0; row < 3; row++) {
        label = lv_label_btn_create(panel, custom_callbacks[row]);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label, "???");
        lv_obj_set_width(label, width);
        lv_obj_add_event_cb(label, position_callbacks[row], LV_EVENT_MSG_RECEIVED, NULL);
        lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

        lv_obj_t * row_panel = lv_create_empty_panel(panel);
        lv_obj_set_size(row_panel, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_layout_flex_row(row_panel);

        for (int col = 0; col < 6; col++)
        {
            btn = lv_btn_create(row_panel);
            lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
            lv_obj_add_event_cb(btn, button_callbacks[row], LV_EVENT_CLICKED, (void*)(offsets[row] + col));
            lv_obj_set_flex_grow(btn, 1);

            label = lv_label_create(btn);
            lv_label_set_text(label, labels[row] + OFFSET_LABEL_SIZE * col);
            lv_obj_center(label);
        }
    }

}

inline void root_panel_steppers_unlocked(lv_obj_t * root_panel){
    lv_obj_t * panel = lv_create_empty_panel(root_panel);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, LV_SYMBOL_EYE_CLOSE " Steppers unlocked");

    lv_obj_t * btn_row = lv_create_empty_panel(panel);
    lv_obj_set_size(btn_row, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(btn_row, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * btn = lv_btn_create(btn_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, home_button_click, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME "Home Axis");
    lv_obj_center(label);

    btn = lv_btn_create(btn_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, switch_to_stat_panel, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_SETTINGS "Parameters");
    lv_obj_center(label);
}

static void root_panel_state_update(lv_event_t * e){
    if (last_homing_state == get_current_printer_data()->homed_axis)
        return;

    lv_obj_t * panel = lv_event_get_target(e);
    last_homing_state = get_current_printer_data()->homed_axis;

    lv_obj_clean(panel);

    if (get_current_printer_data()->homed_axis) 
        root_panel_steppers_locked(panel);
    else 
        root_panel_steppers_unlocked(panel);
}

void move_panel_init(lv_obj_t* panel){
    if (get_current_printer_data()->state == PrinterState::PrinterStatePrinting){
        stats_panel_init(panel);
        return;
    }

    calculate_offsets_from_current_printer();
    last_homing_state = !get_current_printer_data()->homed_axis;

    lv_obj_add_event_cb(panel, root_panel_state_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, panel, NULL);
}