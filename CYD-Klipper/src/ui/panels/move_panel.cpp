#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"
#include "../nav_buttons.h"
#include "../ui_utils.h"
#include <stdio.h>

static bool last_homing_state = false;

static void x_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);
    float data = *data_pointer;
    move_printer("X", data, true);
}

static void y_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);
    float data = *data_pointer;
    move_printer("Y", data, true);
}

static void z_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);
    float data = *data_pointer;
    move_printer("Z", data, true);
}

char x_pos_buff[12];

static void x_pos_update(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(x_pos_buff, "X: %.1f", printer.position[0]);
    lv_label_set_text(label, x_pos_buff);
}

char y_pos_buff[12];

static void y_pos_update(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(y_pos_buff, "Y: %.1f", printer.position[1]);
    lv_label_set_text(label, y_pos_buff);
}

char z_pos_buff[12];

static void z_pos_update(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(z_pos_buff, "Z: %.2f", printer.position[2]);
    lv_label_set_text(label, z_pos_buff);
}

lv_event_cb_t button_callbacks[] = {x_line_button_press, y_line_button_press, z_line_button_press};
lv_event_cb_t position_callbacks[] = {x_pos_update, y_pos_update, z_pos_update};

const float xy_offsets[] = {-100, -10, -1, 1, 10, 100};
const float z_offsets[] = {-10, -1, -0.1, 0.1, 1, 10};
const float* offsets[] = {
    xy_offsets,
    xy_offsets,
    z_offsets
};

const char* xy_offset_labels[] = {"-100", "-10", "-1", "+1", "+10", "+100"};
const char* z_offset_labels[] = {"-10", "-1", "-0.1", "+0.1", "+1", "+10"};

const char** offset_labels[] = {
    xy_offset_labels,
    xy_offset_labels,
    z_offset_labels
};

static void home_button_click(lv_event_t * e) {
    if (printer.state == PRINTER_STATE_PRINTING)
        return;

    send_gcode(false, "G28");
} 

static void disable_steppers_click(lv_event_t * e) {
    if (printer.state == PRINTER_STATE_PRINTING)
        return;

    send_gcode(true, "M18");
} 

static void switch_to_stat_panel(lv_event_t * e) {
    lv_obj_t * panel = lv_event_get_target(e);
    nav_buttons_setup(PANEL_STATS);
}

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
    lv_label_set_text(label, LV_SYMBOL_EYE_CLOSE " Unlock");
    lv_obj_center(label);

    btn = lv_btn_create(home_button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, switch_to_stat_panel, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_EDIT " Params");
    lv_obj_center(label);

    for (int row = 0; row < 3; row++) {
        label = lv_label_create(panel);
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
            lv_label_set_text(label, offset_labels[row][col]);
            lv_obj_center(label);
        }
    }

}

inline void root_panel_steppers_unlocked(lv_obj_t * root_panel){
    lv_obj_t * panel = lv_create_empty_panel(root_panel);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, LV_SYMBOL_EYE_CLOSE " Steppers unlocked");

    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, home_button_click, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME "Home Axis");
    lv_obj_center(label);
}

static void root_panel_state_update(lv_event_t * e){
    if (last_homing_state == printer.homed_axis)
        return;

    lv_obj_t * panel = lv_event_get_target(e);
    last_homing_state = printer.homed_axis;

    lv_obj_clean(panel);

    if (printer.homed_axis) 
        root_panel_steppers_locked(panel);
    else 
        root_panel_steppers_unlocked(panel);
}

void move_panel_init(lv_obj_t* panel){
    if (printer.state == PRINTER_STATE_PRINTING){
        stats_panel_init(panel);
        return;
    }

    last_homing_state = !printer.homed_axis;

    lv_obj_add_event_cb(panel, root_panel_state_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, panel, NULL);
}