#include "bambu_printer_integration.hpp"
#include "lvgl.h"
#include "../../ui/ui_utils.h"
#include <stdio.h>
#include "../common/constants.h"

const char* speed_profiles[] = { "Silent (50%)", "Normal (100%)", "Sport (124%)", "Ludicrous (166%)" };
const BambuSpeedProfile speed_profile_values[] = { BambuSpeedProfileSilent, BambuSpeedProfileNormal, BambuSpeedProfileSport, BambuSpeedProfileLudicrous };

// TODO: Move to common
static void set_fan_speed_text(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    char data[16];
    sprintf(data, "Fan: %.0f%%", get_current_printer_data()->fan_speed * 100);
    lv_label_set_text(label, data);
}

static void set_fan_speed(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    BambuPrinter* printer = (BambuPrinter*)get_current_printer(); // TODO: pass by ref 
    // TODO: Implement
}

FAN_SPEED_COLUMN(set_fan_speed, fan_speed_columns)

static void set_aux_fan_speed_text(lv_event_t * e) {
    // TODO: Implement
    lv_obj_t * label = lv_event_get_target(e);
    char data[16];
    sprintf(data, "Fan: %.0f%%", get_current_printer_data()->fan_speed * 100);
    lv_label_set_text(label, data);
}

static void set_aux_fan_speed(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    BambuPrinter* printer = (BambuPrinter*)get_current_printer(); // TODO: pass by ref 
    // TODO: Implement
}

FAN_SPEED_COLUMN(set_aux_fan_speed, aux_fan_speed_columns)

// TODO: move to common
static void set_speed_mult_text(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char data[16];
    sprintf(data, "Speed: %.0f%%", get_current_printer_data()->speed_mult * 100);
    lv_label_set_text(label, data);
}

static void set_speed_mult(lv_event_t * e)
{
    BambuSpeedProfile speed = (BambuSpeedProfile)lv_event_get_user_data(e);
    BambuPrinter* printer = (BambuPrinter*)get_current_printer(); // TODO: pass by ref 
    // TODO: Implement
}

lv_button_column_t speed_profile_columns[] = {
    { set_speed_mult, speed_profiles, (const void**)speed_profile_values, 4},
};

static void open_fan_speed_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_fan_speed_text, fan_speed_columns, 3);
}

static void open_aux_fan_speed_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_aux_fan_speed_text, aux_fan_speed_columns, 3);
}

static void open_speed_mult_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_speed_mult_text, speed_profile_columns, 1);
}

static PrinterUiPanel bambu_ui_panels[3] {
    { .set_label = (void*)set_fan_speed_text, .open_panel = (void*)open_fan_speed_panel },
    { .set_label = (void*)set_aux_fan_speed_text, .open_panel = (void*)open_aux_fan_speed_panel },
    { .set_label = (void*)set_speed_mult_text, .open_panel = (void*)open_speed_mult_panel },
};

void BambuPrinter::init_ui_panels()
{
    custom_menus_count = 3;
    custom_menus = bambu_ui_panels;
}