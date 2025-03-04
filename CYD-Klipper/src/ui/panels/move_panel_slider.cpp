#include "panel.h"
#include "lvgl.h"
#include "../../core/data_setup.h"
#include "../nav_buttons.h"
#include "../ui_utils.h"
#include "../../core/printer_integration.hpp"
#include "../../core/current_printer.h"

static void line_custom_set(const char * axis, const char *text)
{
    float pos = atof(text);

    if (pos < 0 || pos > 500)
        return;

    current_printer_move_printer(axis, pos, false);
}

static void x_line_custom_callback(lv_event_t * e) 
{
    const char * text = lv_textarea_get_text(lv_event_get_target(e));
    line_custom_set("X", text);
}

static void y_line_custom_callback(lv_event_t * e) 
{
    const char * text = lv_textarea_get_text(lv_event_get_target(e));
    line_custom_set("Y", text);
}

static void z_line_custom_callback(lv_event_t * e) 
{
    const char * text = lv_textarea_get_text(lv_event_get_target(e));
    line_custom_set("Z", text);
}

static void x_line_custom(lv_event_t * e) 
{
    lv_create_keyboard_text_entry(x_line_custom_callback, "Set X position", LV_KEYBOARD_MODE_NUMBER, LV_PCT(75), 6);
}

static void y_line_custom(lv_event_t * e) 
{
    lv_create_keyboard_text_entry(y_line_custom_callback, "Set Y position", LV_KEYBOARD_MODE_NUMBER, LV_PCT(75), 6);
}

static void z_line_custom(lv_event_t * e) 
{
    lv_create_keyboard_text_entry(z_line_custom_callback, "Set Z position", LV_KEYBOARD_MODE_NUMBER, LV_PCT(75), 6);
}

static void pos_update(lv_event_t * e, const char* target_template, float target)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_user_data(e);

    if (lv_slider_is_dragged(slider))
    {
        return;
    }

    if (lv_obj_check_type(obj, &lv_label_class))
    {
        char pos_buff[12];
        sprintf(pos_buff, target_template, target);
        lv_label_set_text(obj, pos_buff);
    }
    else 
    {
        lv_slider_set_value(obj, target, LV_ANIM_ON);
    }
}

static void x_pos_update(lv_event_t * e)
{
    pos_update(e, "%.1f " LV_SYMBOL_EDIT, get_current_printer_data()->position[0]);
} 

static void y_pos_update(lv_event_t * e)
{
    pos_update(e, "%.1f", get_current_printer_data()->position[1]);
}

static void z_pos_update(lv_event_t * e)
{
    pos_update(e, "%.2f", get_current_printer_data()->position[2]);
}

static void x_slider_update(lv_event_t * e)
{
    current_printer_move_printer("X", lv_slider_get_value(lv_event_get_target(e)), false);
} 

static void y_slider_update(lv_event_t * e)
{
    current_printer_move_printer("Y", lv_slider_get_value(lv_event_get_target(e)), false);
}

static void z_slider_update(lv_event_t * e)
{
    current_printer_move_printer("Z", lv_slider_get_value(lv_event_get_target(e)), false);
}

static void home_button_click(lv_event_t * e) 
{
    current_printer_execute_feature(PrinterFeatures::PrinterFeatureHome);
} 

static void disable_steppers_button_click(lv_event_t * e) 
{
    current_printer_execute_feature(PrinterFeatures::PrinterFeatureDisableSteppers);
} 

static void set_label_slider_position(lv_event_t * e)
{
    lv_obj_t* slider = lv_event_get_target(e);
    lv_obj_t* label = (lv_obj_t*)lv_event_get_user_data(e);

    if (lv_slider_is_dragged(slider))
    {
        lv_label_set_text_fmt(label, "%d", lv_slider_get_value(slider));
    }
}

static void switch_to_params_panel_button_click(lv_event_t * e) 
{
    lv_obj_t * panel = lv_event_get_target(e);
    nav_buttons_setup(PANEL_STATS);
}

static void make_vertical_slider(
    lv_obj_t* parent, 
    const char* axis, 
    lv_event_cb_t position_change, 
    lv_event_cb_t on_slider_change, 
    lv_event_cb_t on_edit,
    float min,
    float max)
{
    lv_obj_t* root = lv_create_empty_panel(parent);
    lv_layout_flex_column(root);
    lv_obj_set_size(root, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 1.5, LV_PCT(100));
    lv_obj_set_style_pad_column(root, 10, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* sub_root = lv_create_empty_panel(root);
    lv_layout_flex_column(sub_root);
    lv_obj_set_height(sub_root, LV_SIZE_CONTENT);
    lv_obj_add_flag(sub_root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(sub_root, on_edit, LV_EVENT_CLICKED, NULL);

    lv_obj_t* top_label = lv_label_create(sub_root);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, top_label, NULL);

    lv_obj_t* one_below_label = lv_label_create(sub_root);
    lv_label_set_text(one_below_label, LV_SYMBOL_EDIT);

    lv_obj_t* two_below_label = lv_label_create(sub_root);
    lv_label_set_text_fmt(two_below_label, "%s+", axis);

    lv_obj_t* slider = lv_slider_create(root);
    lv_obj_set_flex_grow(slider, 1);
    lv_obj_set_width(slider, CYD_SCREEN_MIN_BUTTON_WIDTH_PX / 2);
    lv_slider_set_range(slider, min, max);
    lv_obj_add_event_cb(slider, on_slider_change, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(slider, position_change, LV_EVENT_MSG_RECEIVED, slider);
    lv_obj_add_event_cb(slider, set_label_slider_position, LV_EVENT_VALUE_CHANGED, top_label);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, slider, NULL);

    lv_obj_t* last_label = lv_label_create(root);
    lv_label_set_text_fmt(last_label, "%s-", axis);
    lv_obj_add_event_cb(top_label, position_change, LV_EVENT_MSG_RECEIVED, slider);
}

static void make_horizontal_slider(
    lv_obj_t* parent, 
    const char* axis, 
    lv_event_cb_t position_change, 
    lv_event_cb_t on_slider_change, 
    lv_event_cb_t on_edit,
    float min,
    float max)
{
    lv_obj_t* root = lv_create_empty_panel(parent);
    lv_layout_flex_column(root);
    lv_obj_set_size(root, LV_PCT(100), LV_SIZE_CONTENT);

    lv_obj_t* upper = lv_create_empty_panel(root);
    lv_layout_flex_row(upper, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_size(upper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_flag(upper, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(upper, on_edit, LV_EVENT_CLICKED, NULL);

    lv_obj_t* left_label = lv_label_create(upper);
    lv_label_set_text_fmt(left_label, "%s-", axis);

    lv_obj_t* position_label = lv_label_create(upper);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, position_label, NULL);

    lv_obj_t* right_label = lv_label_create(upper);
    lv_label_set_text_fmt(right_label, "%s+", axis);

    lv_obj_t* slider = lv_slider_create(root);
    lv_obj_set_size(slider, LV_PCT(100), CYD_SCREEN_MIN_BUTTON_HEIGHT_PX / 2);
    lv_slider_set_range(slider, min, max);
    lv_obj_add_event_cb(slider, on_slider_change, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(slider, position_change, LV_EVENT_MSG_RECEIVED, slider);
    lv_obj_add_event_cb(slider, set_label_slider_position, LV_EVENT_VALUE_CHANGED, position_label);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, slider, NULL);

    lv_obj_set_style_pad_bottom(root, 10, 0);
    lv_obj_set_style_pad_bottom(upper, 10, 0);
    lv_obj_set_style_pad_column(upper, 10, 0);

    lv_obj_add_event_cb(position_label, position_change, LV_EVENT_MSG_RECEIVED, slider);
}

static void create_button(lv_obj_t* parent, lv_event_cb_t on_click, const char* text)
{
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_width(btn, LV_PCT(100));
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(btn);
    lv_obj_center(label);
    lv_label_set_text(label, text);
}

void move_panel_slider_init(lv_obj_t* panel) 
{
    lv_obj_t * sub_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(sub_panel, LV_PCT(100), LV_PCT(100));

    lv_layout_flex_row(sub_panel);
 
    lv_obj_t* left = lv_create_empty_panel(sub_panel);
    lv_layout_flex_column(left);
    lv_obj_set_flex_grow(left, 1);
    lv_obj_set_height(left, LV_PCT(100));

    float* position_min = get_current_printer_data()->position_min;
    float* position_max = get_current_printer_data()->position_max;

    make_horizontal_slider(left, "X", x_pos_update, x_slider_update, x_line_custom, position_min[0], position_max[0]);
    make_vertical_slider(sub_panel, "Y", y_pos_update, y_slider_update, y_line_custom, position_min[1], position_max[1]);
    make_vertical_slider(sub_panel, "Z", z_pos_update, z_slider_update, z_line_custom, position_min[2], position_max[2]);

    create_button(left, home_button_click, LV_SYMBOL_HOME " Home XYZ");
    create_button(left, disable_steppers_button_click, LV_SYMBOL_EYE_CLOSE " Free Motors");
    create_button(left, switch_to_params_panel_button_click, LV_SYMBOL_SETTINGS " Parameters");

    lv_obj_set_style_pad_right(left, 10, 0);
    lv_obj_set_style_pad_all(sub_panel, 10, 0);
}