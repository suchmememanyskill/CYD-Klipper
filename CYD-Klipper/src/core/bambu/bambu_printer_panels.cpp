#include "bambu_printer_integration.hpp"
#include "lvgl.h"
#include "../../ui/ui_utils.h"
#include <stdio.h>
#include "../common/constants.h"

const char* speed_profiles[] = { "Silent (50%)", "Normal (100%)", "Sport (124%)", "Ludicrous (166%)" };
const BambuSpeedProfile speed_profile_values[] = { BambuSpeedProfileSilent, BambuSpeedProfileNormal, BambuSpeedProfileSport, BambuSpeedProfileLudicrous };
const char* COMMAND_SET_PRINT_SPEED = "{\"print\":{\"command\":\"print_speed\",\"param\":\"%d\"}}";
const char* COMMAND_START_PRINT_3MF = "{\"print\":{\"command\":\"project_file\",\"param\":\"Metadata/plate_1.gcode\",\"project_id\":\"0\",\"profile_id\":\"0\",\"task_id\":\"0\",\"subtask_id\":\"0\",\"subtask_name\":\"CYD-Klipper Print Job\",\"url\":\"file:///sdcard/%s\",\"timelapse\":%s,\"bed_type\":\"auto\",\"bed_leveling\":%s,\"flow_cali\":%s,\"vibration_cali\":%s,\"layer_inspect\":%s,\"ams_mapping\":[],\"use_ams\":%s}}";

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

struct 
{
    const char* bambu_current_file;

    union 
    {
        struct 
        {
            bool bambu_option_use_ams : 1;
            bool bambu_option_timelapse : 1;
            bool bambu_option_bed_leveling : 1;
            bool bambu_option_flow_calibration : 1;
            bool bambu_option_vibration_compensation : 1;
            bool bambu_option_layer_inspect : 1;        
        };
        unsigned char bambu_options_raw;
    };
} __internal_bambu_file_state = {};

#define SET_BOOL_STATE(bool_name, func_name) static void func_name (lv_event_t * e) { bool_name = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED); }

SET_BOOL_STATE(__internal_bambu_file_state.bambu_option_use_ams, set_bambu_option_use_ams)
SET_BOOL_STATE(__internal_bambu_file_state.bambu_option_timelapse, set_bambu_option_timelapse)
SET_BOOL_STATE(__internal_bambu_file_state.bambu_option_bed_leveling, set_bambu_option_bed_leveling)
SET_BOOL_STATE(__internal_bambu_file_state.bambu_option_flow_calibration, set_bambu_option_flow_calibration)
SET_BOOL_STATE(__internal_bambu_file_state.bambu_option_vibration_compensation, set_bambu_option_vibration_compensation)
SET_BOOL_STATE(__internal_bambu_file_state.bambu_option_layer_inspect, set_bambu_option_layer_inspect)

#define BOOLEAN_TO_STRING(b) b ? "true" : "false"

static void print_file_start(lv_event_t * e)
{
    BambuPrinter* printer = (BambuPrinter*)get_current_printer();
    char buff[713];

    if (snprintf(buff, 713, COMMAND_START_PRINT_3MF, 
        __internal_bambu_file_state.bambu_current_file,
        BOOLEAN_TO_STRING(__internal_bambu_file_state.bambu_option_timelapse),
        BOOLEAN_TO_STRING(__internal_bambu_file_state.bambu_option_bed_leveling),
        BOOLEAN_TO_STRING(__internal_bambu_file_state.bambu_option_flow_calibration),
        BOOLEAN_TO_STRING(__internal_bambu_file_state.bambu_option_vibration_compensation),
        BOOLEAN_TO_STRING(__internal_bambu_file_state.bambu_option_layer_inspect),
        BOOLEAN_TO_STRING(__internal_bambu_file_state.bambu_option_use_ams)) >= 712)
        {
            LOG_LN("Failed to prepare message to start print");
            return;
        }

    printer->publish_mqtt_command(buff);
}

bool BambuPrinter::start_file(const char* filename){
    if (get_current_printer_data()->state != PrinterState::PrinterStateIdle)
    {
        return false;
    }

    __internal_bambu_file_state.bambu_current_file = filename;
    
    lv_obj_t * panel = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX * 2, 0);
    lv_layout_flex_column(panel);
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * label_print_file = lv_label_create(panel);
    lv_obj_set_width(label_print_file, LV_PCT(100));
    lv_label_set_long_mode(label_print_file, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text_fmt(label_print_file, "Settings for %s", __internal_bambu_file_state.bambu_current_file);

    __internal_bambu_file_state.bambu_option_use_ams = ((BambuPrinter*)get_current_printer())->has_ams;
    __internal_bambu_file_state.bambu_option_timelapse = false;
    __internal_bambu_file_state.bambu_option_bed_leveling = true;
    __internal_bambu_file_state.bambu_option_flow_calibration = true;
    __internal_bambu_file_state.bambu_option_vibration_compensation = true;
    __internal_bambu_file_state.bambu_option_layer_inspect = true;      

    if (__internal_bambu_file_state.bambu_option_use_ams)
    {
        lv_create_custom_menu_switch("Use AMS", panel, set_bambu_option_use_ams, __internal_bambu_file_state.bambu_option_use_ams);
    }

    lv_create_custom_menu_switch("Timelapse", panel, set_bambu_option_timelapse, __internal_bambu_file_state.bambu_option_timelapse);
    lv_create_custom_menu_switch("Bed Leveling", panel, set_bambu_option_bed_leveling, __internal_bambu_file_state.bambu_option_bed_leveling);
    lv_create_custom_menu_switch("Flow Calibration", panel, set_bambu_option_flow_calibration, __internal_bambu_file_state.bambu_option_flow_calibration);
    lv_create_custom_menu_switch("Vibration Compensation", panel, set_bambu_option_vibration_compensation, __internal_bambu_file_state.bambu_option_vibration_compensation);
    lv_create_custom_menu_switch("Inspect First Layer", panel, set_bambu_option_layer_inspect, __internal_bambu_file_state.bambu_option_layer_inspect);

    lv_obj_t* buttons_panel = lv_create_empty_panel(panel);
    lv_layout_flex_row(buttons_panel);
    lv_obj_set_size(buttons_panel, LV_PCT(100), CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t* btn = lv_btn_create(buttons_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, panel);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Cancel");
    lv_obj_center(label);

    btn = lv_btn_create(buttons_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, print_file_start, LV_EVENT_CLICKED, panel);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_OK " Print");
    lv_obj_center(label);

    return true;
}