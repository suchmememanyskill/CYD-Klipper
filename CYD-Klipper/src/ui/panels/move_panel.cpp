#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"
#include <TFT_eSPI.h>
#include "../ui_utils.h"

static bool last_homing_state = false;

static void move_printer(const char* axis, float amount) {
    if (!printer.homed_axis || printer.state == PRINTER_STATE_PRINTING)
        return;

    char gcode[64];
    const char* extra = (amount > 0) ? "+" : "";

    bool absolute_coords = printer.absolute_coords;

    if (absolute_coords) {
        send_gcode(true, "G91");
    }

    const char * space = "%20";

    sprintf(gcode, "G1%s%s%s%.1f%sF6000", space, axis, extra, amount, space);
    send_gcode(true, gcode);

    if (absolute_coords) {
        send_gcode(true, "G90");
    }
}

static void x_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);
    float data = *data_pointer;
    move_printer("X", data);
}

static void y_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);
    float data = *data_pointer;
    move_printer("Y", data);
}

static void z_line_button_press(lv_event_t * e) {
    float* data_pointer = (float*)lv_event_get_user_data(e);
    float data = *data_pointer;
    move_printer("Z", data);
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

static void stepper_state_update(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, printer.homed_axis ? LV_SYMBOL_HOME " Steppers locked" : LV_SYMBOL_EYE_CLOSE " Steppers unlocked");
}

inline void root_panel_steppers_locked(lv_obj_t * root_panel){
    const auto width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_BIG_GAP_PX * 2;

    lv_obj_t * panel = lv_create_empty_panel(root_panel);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_BIG_GAP_PX, 0);
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_SPACE_BETWEEN, 0, 0);

    lv_obj_t * home_button_row = lv_create_empty_panel(panel);
    lv_obj_set_size(home_button_row, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(home_button_row);

    lv_obj_t * btn = lv_btn_create(home_button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, home_button_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME "Home Axis");
    lv_obj_center(label);

    btn = lv_btn_create(home_button_row);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, disable_steppers_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_EYE_CLOSE " Disable Step");
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
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_BIG_GAP_PX, 0);
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
    last_homing_state = !printer.homed_axis;

    lv_obj_add_event_cb(panel, root_panel_state_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, panel, NULL);

    return;

    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    const int button_size = 40;
    const int button_size_vertical = 40;
    const int button_padding = 2;
    const int x_offset = 15;
    int y_pos = 75;

    auto panel_width = TFT_HEIGHT - 40;

    lv_obj_t * home_button = lv_btn_create(panel);
    lv_obj_align(home_button, LV_ALIGN_TOP_LEFT, 10, 5);
    lv_obj_add_event_cb(home_button, home_button_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(home_button, panel_width / 2 - 15, 30);

    lv_obj_t * home_label = lv_label_create(home_button);
    lv_label_set_text(home_label, LV_SYMBOL_HOME "Home Axis");
    lv_obj_center(home_label);

    lv_obj_t * disable_steppers_button = lv_btn_create(panel);
    lv_obj_align(disable_steppers_button, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_event_cb(disable_steppers_button, disable_steppers_click, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(disable_steppers_button, panel_width / 2 - 15, 30);

    lv_obj_t * disable_steppers_label = lv_label_create(disable_steppers_button);
    lv_label_set_text(disable_steppers_label, LV_SYMBOL_EYE_CLOSE "Disable Step");
    lv_obj_center(disable_steppers_label);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_event_cb(label, stepper_state_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    for (int i = 0; i < 3; i++) {
        lv_obj_t * btn = lv_btn_create(panel);
        lv_obj_set_size(btn, button_size, button_size_vertical);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_offset, y_pos);
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, (void*)(offsets[i]));

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, offset_labels[i][0]);
        lv_obj_center(label);

        btn = lv_btn_create(panel);
        lv_obj_set_size(btn, button_size, button_size_vertical);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_offset + (button_size + button_padding) * 1, y_pos);
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, (void*)(offsets[i] + 1));

        label = lv_label_create(btn);
        lv_label_set_text(label, offset_labels[i][1]);
        lv_obj_center(label);

        btn = lv_btn_create(panel);
        lv_obj_set_size(btn, button_size, button_size_vertical);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_offset + (button_size + button_padding) * 2, y_pos);
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, (void*)(offsets[i] + 2));

        label = lv_label_create(btn);
        lv_label_set_text(label, offset_labels[i][2]);
        lv_obj_center(label);

        btn = lv_btn_create(panel);
        lv_obj_set_size(btn, button_size, button_size_vertical);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_offset + (button_size + button_padding) * 3, y_pos);
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, (void*)(offsets[i] + 3));

        label = lv_label_create(btn);
        lv_label_set_text(label, offset_labels[i][3]);
        lv_obj_center(label);

        btn = lv_btn_create(panel);
        lv_obj_set_size(btn, button_size, button_size_vertical);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_offset + (button_size + button_padding) * 4, y_pos);
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, (void*)(offsets[i] + 4));

        label = lv_label_create(btn);
        lv_label_set_text(label, offset_labels[i][4]);
        lv_obj_center(label);

        btn = lv_btn_create(panel);
        lv_obj_set_size(btn, button_size, button_size_vertical);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_offset + (button_size + button_padding) * 5, y_pos);
        lv_obj_add_event_cb(btn, button_callbacks[i], LV_EVENT_CLICKED, (void*)(offsets[i] + 5));
        
        label = lv_label_create(btn);
        lv_label_set_text(label, offset_labels[i][5]);
        lv_obj_center(label);

        label = lv_label_create(panel);
        lv_label_set_text(label, "???");
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, x_offset, y_pos - 15);\
        lv_obj_add_event_cb(label, position_callbacks[i], LV_EVENT_MSG_RECEIVED, NULL);
        lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

        y_pos += 60;
    }
}