#include "panel.h"
#include "../ui_utils.h"
#include "../../core/data_setup.h"
#include <stdio.h>
#include <Esp.h>

static void set_fan_speed_text(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Fan: %.0f%%", printer.fan_speed * 100);
    lv_label_set_text(label, data);
}

static void set_fan_speed(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[64];
    sprintf(gcode, "M106 S%d", speed);
    send_gcode(true, gcode);
}

const char* fan_speeds[] = { "0%", "15%", "25%", "35%" };
const int fan_speeds_values[] = { 0, 38, 64, 90 };

const char* fan_speeds_2[] = { "50%", "75%", "100%"};
const int fan_speeds_values_2[] = { 128, 192, 255 };

lv_button_column_t fan_speed_columns[] = {
    { set_fan_speed, fan_speeds, (const void**)fan_speeds_values, 4},
    { set_fan_speed, fan_speeds_2, (const void**)fan_speeds_values_2, 3}
};

static void set_zoffset_text(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Z Offset: %.03f", printer.gcode_offset[2]);
    lv_label_set_text(label, data);
}

static void set_zoffset(lv_event_t * e){
    char* offset = (char*)lv_event_get_user_data(e);
    char gcode[64];
    sprintf(gcode, "SET_GCODE_OFFSET Z_ADJUST=%s", offset);
    send_gcode(true, gcode);
}

const char* zoffsets[] = { "-0.005", "-0.01", "-0.025", "-0.05" };
const char* zoffsets_2[] = { "+0.005", "+0.01", "+0.025", "+0.05" };

lv_button_column_t zoffset_columns[] = {
    { set_zoffset, zoffsets, (const void**)zoffsets, 4},
    { set_zoffset, zoffsets_2, (const void**)zoffsets_2, 4}
};

static void set_speed_mult_text(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Speed: %.0f%%", printer.speed_mult * 100);
    lv_label_set_text(label, data);
}

static void set_speed_mult(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[64];
    sprintf(gcode, "M220 S%d", speed);
    send_gcode(true, gcode);
}

static void set_speed_mult_offset(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    float result = printer.speed_mult * 100 + speed;
    printer.speed_mult = result / 100;
    char gcode[64];
    sprintf(gcode, "M220 S%.0f", result);
    send_gcode(true, gcode);
}

const char* speed_presets[] = { "50%", "100%", "150%", "200%" };
const int speed_presets_values[] = { 50, 100, 150, 200 };
const char* speed_presets_minus[] = { "-1%", "-5%", "-10%", "-25%" };
const int speed_presets_minus_values[] = { -1, -5, -10, -25 };
const char* speed_presets_plus[] = { "+1%", "+5%", "+10%", "+25%" };
const int speed_presets_plus_values[] = { 1, 5, 10, 25 };

lv_button_column_t speed_mult_columns[] = {
    { set_speed_mult, speed_presets, (const void**)speed_presets_values, 4},
    { set_speed_mult_offset, speed_presets_minus, (const void**)speed_presets_minus_values, 4},
    { set_speed_mult_offset, speed_presets_plus, (const void**)speed_presets_plus_values, 4}
};

static void set_extrude_mult_text(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Flow: %.0f%%", printer.extrude_mult * 100);
    lv_label_set_text(label, data);
}

static void set_extrude_mult(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[64];
    sprintf(gcode, "M221 S%d", speed);
    send_gcode(true, gcode);
}

static void set_extrude_mult_offset(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    float result = printer.extrude_mult * 100 + speed;
    printer.extrude_mult = result / 100;
    char gcode[64];
    sprintf(gcode, "M221 S%.0f", result);
    send_gcode(true, gcode);
}

const char* extrude_presets[] = { "95%", "100%", "105%", "110%" };
const int extrude_presets_values[] = { 95, 100, 105, 110 };
const char* extrude_offset[] = { "+5%", "+1%", "-1%", "-5%" };
const int extrude_offset_values[] = { 5, 1, -1, -5 };

lv_button_column_t extrude_mult_columns[] = {
    { set_extrude_mult, extrude_presets, (const void**)extrude_presets_values, 4},
    { set_extrude_mult_offset, extrude_offset, (const void**)extrude_offset_values, 4}
};

static void open_fan_speed_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_fan_speed_text, fan_speed_columns, 2);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

static void open_zoffset_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_zoffset_text, zoffset_columns, 2);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

static void open_speed_mult_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_speed_mult_text, speed_mult_columns, 3);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

static void open_extrude_mult_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_extrude_mult_text, extrude_mult_columns, 2);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

void create_state_button(lv_obj_t * root, lv_event_cb_t label, lv_event_cb_t button){
    lv_obj_t * btn = lv_btn_create(root);
    lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX / 2 - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, button, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_obj = lv_label_create_ex(btn);
    lv_obj_add_event_cb(label_obj, label, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, label_obj, NULL);
    lv_obj_align(label_obj, LV_ALIGN_CENTER, 0, 0);
}

static void label_pos(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char x_pos_buff[32];
    sprintf(x_pos_buff, "X%.2f Y%.2f", printer.position[0], printer.position[1]);
    lv_label_set_text(label, x_pos_buff);
}

static void label_filament_used_m(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char filament_buff[32];
    sprintf(filament_buff, "%.2f m", printer.filament_used_mm / 1000);
    lv_label_set_text(label, filament_buff);
}

static void label_total_layers(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char layers_buff[32];
    sprintf(layers_buff, "%d of %d", printer.current_layer, printer.total_layers);
    lv_label_set_text(label, layers_buff);
}

static void label_pressure_advance(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char pressure_buff[32];
    sprintf(pressure_buff, "%.3f", printer.pressure_advance);
    lv_label_set_text(label, pressure_buff);
}

static void label_feedrate(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char feedrate_buff[32];
    sprintf(feedrate_buff, "%d mm/s", printer.feedrate_mm_per_s);
    lv_label_set_text(label, feedrate_buff);
}

void create_stat_text_block(lv_obj_t * root, const char* label, lv_event_cb_t value){
    lv_obj_t * panel = lv_create_empty_panel(root);
    lv_obj_set_size(panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_layout_flex_column(panel , LV_FLEX_ALIGN_START, CYD_SCREEN_GAP_PX / 2, CYD_SCREEN_GAP_PX / 2);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t * label_obj = lv_label_create_ex(panel);
    lv_label_set_text(label_obj, label);
    lv_obj_set_style_text_font(label_obj, CYD_SCREEN_FONT_SMALL, 0);

    lv_obj_t * value_obj = lv_label_create_ex(panel);
    lv_obj_add_event_cb(value_obj, value, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, value_obj, NULL);
}

void stats_panel_init(lv_obj_t* panel) {
    auto panel_width = CYD_SCREEN_PANEL_WIDTH_PX / 2 - CYD_SCREEN_GAP_PX * 3;

    lv_obj_t * left_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(left_panel, panel_width, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_column(left_panel);
    lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_align(left_panel, LV_ALIGN_TOP_LEFT, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

    create_stat_text_block(left_panel, "Position:", label_pos);
    create_stat_text_block(left_panel, "Filament Used:", label_filament_used_m);
    create_stat_text_block(left_panel, "Layer:", label_total_layers);
    create_stat_text_block(left_panel, "Pressure Advance:", label_pressure_advance);
    create_stat_text_block(left_panel, "Feedrate:", label_feedrate);

    lv_obj_t * right_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(right_panel, panel_width, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_column(right_panel, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(right_panel, LV_ALIGN_TOP_RIGHT, -1 * CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);
    
    create_state_button(right_panel, set_fan_speed_text, open_fan_speed_panel);
    create_state_button(right_panel, set_zoffset_text, open_zoffset_panel);
    create_state_button(right_panel, set_speed_mult_text, open_speed_mult_panel);
    create_state_button(right_panel, set_extrude_mult_text, open_extrude_mult_panel);
}