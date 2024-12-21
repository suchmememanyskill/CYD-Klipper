#include "bambu_printer_integration.hpp"
#include "lvgl.h"
#include "../../ui/ui_utils.h"
#include <stdio.h>
#include "../common/constants.h"

const char* speed_profiles[] = { "Silent (50%)", "Normal (100%)", "Sport (124%)", "Ludicrous (166%)" };
const BambuSpeedProfile speed_profile_values[] = { BambuSpeedProfileSilent, BambuSpeedProfileNormal, BambuSpeedProfileSport, BambuSpeedProfileLudicrous };
const char* COMMAND_SET_PRINT_SPEED = "{\"print\":{\"command\":\"print_speed\",\"param\":\"%d\"}}";

enum FanIndex
{
    PartFan = 1,
    AuxFan = 2,
    ChamberFan = 3,
};

static void set_fan_speed_text(lv_event_t* e, FanIndex index)
{
    lv_obj_t * label = lv_event_get_target(e);
    char data[16];

    float fan_speed = 0;
    const char* fan_type = "";

    switch (index)
    {
        case PartFan:
            fan_speed = get_current_printer_data()->fan_speed;
            fan_type = "P.Fan";
            break;
        case AuxFan:
            fan_speed = ((BambuPrinter*)get_current_printer)->aux_fan_speed;
            fan_type = "A.Fan";
            break;
        case ChamberFan:
            fan_speed = ((BambuPrinter*)get_current_printer)->chamber_fan_speed;
            fan_type = "C.Fan";
            break;
    }

    sprintf(data, "%s: %.0f%%", fan_type, get_current_printer_data()->fan_speed * 100);
    lv_label_set_text(label, data);
}

static void set_fan_speed(lv_event_t* e, FanIndex index)
{
    int speed = (int)lv_event_get_user_data(e);
    int actual_speed = fan_percent_to_byte(speed);
    BambuPrinter* printer = (BambuPrinter*)get_current_printer(); // TODO: pass by ref 
    char buff[20];
    sprintf(buff, "M106 P%d S%d", index, actual_speed);
    printer->send_gcode(buff);
}

static void set_part_fan_speed_text(lv_event_t * e) 
{
    set_fan_speed_text(e, FanIndex::PartFan);
}

static void set_part_fan_speed(lv_event_t * e)
{
    set_fan_speed(e, FanIndex::PartFan);
}

FAN_SPEED_COLUMN(set_part_fan_speed, part_fan_speed_columns)

static void set_aux_fan_speed_text(lv_event_t * e) 
{
    set_fan_speed_text(e, FanIndex::AuxFan);
}

static void set_aux_fan_speed(lv_event_t * e)
{
    set_fan_speed(e, FanIndex::AuxFan);
}

FAN_SPEED_COLUMN(set_aux_fan_speed, aux_fan_speed_columns)

static void set_chamber_fan_speed_text(lv_event_t * e) 
{
    set_fan_speed_text(e, FanIndex::ChamberFan);
}

static void set_chamber_fan_speed(lv_event_t * e)
{
    set_fan_speed(e, FanIndex::ChamberFan);
}

FAN_SPEED_COLUMN(set_chamber_fan_speed, chamber_fan_speed_columns)

// TODO: move to common
static void set_speed_mult_text(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char buff[16];
    sprintf(buff, "Speed: %.0f%%", get_current_printer_data()->speed_mult * 100);
    lv_label_set_text(label, buff);
}

static void set_speed_mult(lv_event_t * e)
{
    BambuSpeedProfile speed = (BambuSpeedProfile)((int)lv_event_get_user_data(e));
    BambuPrinter* printer = (BambuPrinter*)get_current_printer(); // TODO: pass by ref 
    char buff[128];

    sprintf(buff, COMMAND_SET_PRINT_SPEED, speed);
    printer->publish_mqtt_command(buff);
}

lv_button_column_t speed_profile_columns[] = {
    { set_speed_mult, speed_profiles, (const void**)speed_profile_values, 4},
};

static void open_part_fan_speed_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_part_fan_speed_text, part_fan_speed_columns, 3);
}

static void open_aux_fan_speed_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_aux_fan_speed_text, aux_fan_speed_columns, 3);
}

static void open_chamber_fan_speed_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_chamber_fan_speed_text, chamber_fan_speed_columns, 3);
}


static void open_speed_mult_panel(lv_event_t * e){
    lv_create_fullscreen_button_matrix_popup(lv_scr_act(), set_speed_mult_text, speed_profile_columns, 1);
}

static PrinterUiPanel bambu_ui_panels[4] {
    { .set_label = (void*)set_speed_mult_text, .open_panel = (void*)open_speed_mult_panel },
    { .set_label = (void*)set_part_fan_speed_text, .open_panel = (void*)open_part_fan_speed_panel },
    { .set_label = (void*)set_chamber_fan_speed_text, .open_panel = (void*)open_chamber_fan_speed_panel },
    { .set_label = (void*)set_aux_fan_speed_text, .open_panel = (void*)open_aux_fan_speed_panel },
};

void BambuPrinter::init_ui_panels()
{
    custom_menus_count = 4;
    custom_menus = bambu_ui_panels;
}