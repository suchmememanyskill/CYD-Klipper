#include "klipper_printer_integration.hpp"
#include "lvgl.h"
#include "../../ui/ui_utils.h"
#include "../common/constants.h"
#include <stdio.h>
#include "../semaphore.h"

bool send_gcode_blocking(const char *gcode, bool wait = true)
{
    freeze_request_thread();
    KlipperPrinter* printer = (KlipperPrinter*)get_current_printer(); // TODO: pass by ref
    bool result = printer->send_gcode(gcode);
    unfreeze_request_thread();

    return result;
}

bool move_printer_blocking(const char* axis, float amount, bool relative)
{
    freeze_request_thread();
    KlipperPrinter* printer = (KlipperPrinter*)get_current_printer(); // TODO: pass by ref
    bool result = printer->move_printer(axis, amount, relative);
    unfreeze_request_thread();

    return result;
}

static void set_fan_speed_text(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    char data[16];
    sprintf(data, "Fan: %.0f%%", get_current_printer_data()->fan_speed * 100);
    lv_label_set_text(label, data);
}

static void set_fan_speed(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    int actual_speed = fan_percent_to_byte(speed);
    char gcode[16];
    sprintf(gcode, "M106 S%d", actual_speed);
    send_gcode_blocking(gcode);
}

FAN_SPEED_COLUMN(set_fan_speed, klipper_fan_speed_columns)

static void set_zoffset_text(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    KlipperPrinter* printer = (KlipperPrinter*)get_current_printer(); // TODO: pass by ref
    char data[24];
    sprintf(data, "Z Offset: %.03f", printer->gcode_offset[2]);
    lv_label_set_text(label, data);
}

static void set_zoffset_text_ex(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    KlipperPrinter* printer = (KlipperPrinter*)get_current_printer(); // TODO: pass by ref
    char data[32];
    sprintf(data, "Z Offset: %.03f, Z: %.03f", printer->gcode_offset[2], get_current_printer_data()->position[2]);
    lv_label_set_text(label, data);
}

static void set_zoffset(lv_event_t * e){
    char* offset = (char*)lv_event_get_user_data(e);
    char gcode[48];
    sprintf(gcode, "SET_GCODE_OFFSET Z_ADJUST=%s MOVE=1", offset);
    send_gcode_blocking(gcode);
}

static void set_z(lv_event_t * e){
    void* ptr = lv_event_get_user_data(e);
    float value = *(float *)(&ptr);

    if (value < 0) {
        send_gcode_blocking("SET_GCODE_OFFSET Z=0 MOVE=1");
        return;
    }

    move_printer_blocking("Z", value, false);
}

const char* zoffsets[] = { "-0.01", "-0.025", "-0.05", "-0.2" };
const char* zoffsets_2[] = { "+0.01", "+0.025", "+0.05", "+0.2" };
const char* zabs[] = { "Z=0", "Z=0.1", "Z=1", "Clear" };
const float zabsvalues[] = { 0, 0.1f, 1.0f, -1.0f };

lv_button_column_t zoffset_columns[] = {
    { set_zoffset, zoffsets, (const void**)zoffsets, 4},
    { set_zoffset, zoffsets_2, (const void**)zoffsets_2, 4},
    { set_z, zabs, (const void**)zabsvalues, 4}
};

static void set_speed_mult_text(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char data[16];
    sprintf(data, "Speed: %.0f%%", get_current_printer_data()->speed_mult * 100);
    lv_label_set_text(label, data);
}

static void set_speed_mult(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[16];
    sprintf(gcode, "M220 S%d", speed);
    send_gcode_blocking(gcode);
}

static void set_speed_mult_offset(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    float result = get_current_printer_data()->speed_mult * 100 + speed;
    get_current_printer_data()->speed_mult = result / 100;
    char gcode[16];
    sprintf(gcode, "M220 S%.0f", result);
    send_gcode_blocking(gcode);
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
    char data[16];
    sprintf(data, "Flow: %.0f%%", get_current_printer_data()->extrude_mult * 100);
    lv_label_set_text(label, data);
}

static void set_extrude_mult(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[16];
    sprintf(gcode, "M221 S%d", speed);
    send_gcode_blocking(gcode);
}

static void set_extrude_mult_offset(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    float result = get_current_printer_data()->extrude_mult * 100 + speed;
    get_current_printer_data()->extrude_mult = result / 100;
    char gcode[16];
    sprintf(gcode, "M221 S%.0f", result);
    
    send_gcode_blocking(gcode);
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
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_fan_speed_text, klipper_fan_speed_columns, 3);
}

static void open_zoffset_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_zoffset_text_ex, zoffset_columns, get_current_printer_data()->state == PrinterStateIdle ? 3 : 2);
}

static void open_speed_mult_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_speed_mult_text, speed_mult_columns, 3);
}

static void open_extrude_mult_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_extrude_mult_text, extrude_mult_columns, 2);
}

static PrinterUiPanel klipper_ui_panels[4] {
    { .set_label = (void*)set_fan_speed_text, .open_panel = (void*)open_fan_speed_panel },
    { .set_label = (void*)set_zoffset_text, .open_panel = (void*)open_zoffset_panel },
    { .set_label = (void*)set_speed_mult_text, .open_panel = (void*)open_speed_mult_panel },
    { .set_label = (void*)set_extrude_mult_text, .open_panel = (void*)open_extrude_mult_panel }
};

void KlipperPrinter::init_ui_panels()
{
    custom_menus_count = 4;
    custom_menus = klipper_ui_panels;
}