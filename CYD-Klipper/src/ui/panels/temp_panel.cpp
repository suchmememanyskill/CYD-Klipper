#include "lvgl.h"
#include "../../core/data_setup.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include "../ui_utils.h"

enum temp_target{
    TARGET_HOTEND,
    TARGET_BED,
    TARGET_HOTEND_CONFIG_1,
    TARGET_HOTEND_CONFIG_2,
    TARGET_HOTEND_CONFIG_3,
    TARGET_BED_CONFIG_1,
    TARGET_BED_CONFIG_2,
    TARGET_BED_CONFIG_3,
};

static temp_target keyboard_target;
static char hotend_buff[40];
static char bed_buff[40];
static bool edit_mode = false;
lv_obj_t* root_panel;

static void update_printer_data_hotend_temp(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(hotend_buff, "Hotend: %.0f C (Target: %.0f C)", printer.extruder_temp, printer.extruder_target_temp);
    lv_label_set_text(label, hotend_buff);
}

static void update_printer_data_bed_temp(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(bed_buff, "Bed: %.0f C (Target: %.0f C)", printer.bed_temp, printer.bed_target_temp);
    lv_label_set_text(label, bed_buff);
}

static short get_temp_preset(int target){
    switch (target){
        case TARGET_HOTEND_CONFIG_1:
            return global_config.hotend_presets[0];
        case TARGET_HOTEND_CONFIG_2:
            return global_config.hotend_presets[1];
        case TARGET_HOTEND_CONFIG_3:
            return global_config.hotend_presets[2];
        case TARGET_BED_CONFIG_1:
            return global_config.bed_presets[0];
        case TARGET_BED_CONFIG_2:
            return global_config.bed_presets[1];
        case TARGET_BED_CONFIG_3:
            return global_config.bed_presets[2];
        default:
            return -1;
    }
}

static void update_temp_preset_label(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    int target = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    short value = get_temp_preset(target);

    String text_label = String(value) + " C";
    lv_label_set_text(label, text_label.c_str());
}

void UpdateConfig(){
    WriteGlobalConfig();
    lv_msg_send(DATA_PRINTER_TEMP_PRESET, &printer);
}

static void keyboard_callback(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_READY) {
        const char * text = lv_textarea_get_text(ta);

        int temp = atoi(text);
        if (temp < 0 || temp > 500){
            return;
        }

        char gcode[64];
        
        switch (keyboard_target){
            case TARGET_HOTEND:
                sprintf(gcode, "M104 S%d", temp);
                send_gcode(true, gcode);
                break;
            case TARGET_BED:
                sprintf(gcode, "M140 S%d", temp);
                send_gcode(true, gcode);
                break;
            case TARGET_HOTEND_CONFIG_1:
                global_config.hotend_presets[0] = temp;
                UpdateConfig();
                break;
            case TARGET_HOTEND_CONFIG_2:
                global_config.hotend_presets[1] = temp;
                UpdateConfig();
                break;
            case TARGET_HOTEND_CONFIG_3:
                global_config.hotend_presets[2] = temp;
                UpdateConfig();
                break;
            case TARGET_BED_CONFIG_1:
                global_config.bed_presets[0] = temp;
                UpdateConfig();
                break;
            case TARGET_BED_CONFIG_2:
                global_config.bed_presets[1] = temp;
                UpdateConfig();
                break;
            case TARGET_BED_CONFIG_3:
                global_config.bed_presets[2] = temp;
                UpdateConfig();
                break;
        }
    }

    if(code == LV_EVENT_DEFOCUSED || code == LV_EVENT_CANCEL || code == LV_EVENT_READY) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_del(kb);
        lv_obj_del(ta);
    }
}

static void show_keyboard(lv_event_t * e){
    lv_obj_t * keyboard = lv_keyboard_create(root_panel);
    lv_obj_t * ta = lv_textarea_create(root_panel);
    // TODO: Hack, should be fixed before finishing porting
    lv_obj_set_size(ta, CYD_SCREEN_PANEL_WIDTH_PX, 120);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 0);
    lv_textarea_set_max_length(ta, 3);
    //lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, "");
    lv_textarea_set_align(ta, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_event_cb(ta, keyboard_callback, LV_EVENT_ALL, keyboard);

    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, ta);
}

static void show_keyboard_with_hotend(lv_event_t * e){
    keyboard_target = TARGET_HOTEND;
    show_keyboard(e);
}

static void show_keyboard_with_bed(lv_event_t * e){
    keyboard_target = TARGET_BED;
    show_keyboard(e);
}

static void cooldown_temp(lv_event_t * e){
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }
    
    send_gcode(true, "M104 S0");
    send_gcode(true, "M140 S0");
}

static void btn_extrude(lv_event_t * e){
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }

    send_gcode(true, "M83");
    send_gcode(true, "G1 E25 F300");
}

static void set_temp_via_preset(lv_event_t * e){
    int target = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    int value = get_temp_preset(target);

    if (edit_mode) {
        keyboard_target = (temp_target)target;
        show_keyboard(e);
        return;
    }

    char gcode[64];
    if (target <= TARGET_HOTEND_CONFIG_3)
        sprintf(gcode, "M104 S%d", value);
    else 
        sprintf(gcode, "M140 S%d", value);
    
    send_gcode(true, gcode);
}

static void btn_toggleable_edit(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    auto state = lv_obj_get_state(btn);
    edit_mode = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
}

static void btn_retract(lv_event_t * e){
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }

    send_gcode(true, "M83");
    send_gcode(true, "G1 E-25 F300");
}

void temp_panel_init(lv_obj_t * panel){
    const auto element_width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;
    root_panel = panel;
    edit_mode = false;

    lv_obj_t * root_temp_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(root_temp_panel, CYD_SCREEN_PANEL_WIDTH_PX, LV_SIZE_CONTENT);
    lv_obj_align(root_temp_panel, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_pad_all(root_temp_panel, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(root_temp_panel);

    lv_obj_t * temp_rows[2] = {0};
    lv_obj_t * button_temp_rows[2] = {0};

    for (int tempIter = 0; tempIter < 2; tempIter++){
        temp_rows[tempIter] = lv_create_empty_panel(root_temp_panel);
        lv_layout_flex_column(temp_rows[tempIter]);
        lv_obj_set_size(temp_rows[tempIter], element_width, LV_SIZE_CONTENT);

        lv_obj_t * label = lv_label_create(temp_rows[tempIter]);
        lv_label_set_text(label, "???");
        lv_obj_add_event_cb(label, (tempIter == 0) ? update_printer_data_hotend_temp : update_printer_data_bed_temp, LV_EVENT_MSG_RECEIVED, NULL);
        lv_msg_subscribe_obj(DATA_PRINTER_DATA, label, NULL);
        lv_obj_set_width(label, element_width);

        button_temp_rows[tempIter] = lv_create_empty_panel(temp_rows[tempIter]);
        lv_layout_flex_row(button_temp_rows[tempIter], LV_FLEX_ALIGN_SPACE_EVENLY);
        lv_obj_set_size(button_temp_rows[tempIter], element_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        for (int buttonIter = 0; buttonIter < 3; buttonIter++){
            lv_obj_t * btn = lv_btn_create(button_temp_rows[tempIter]);
            lv_obj_add_event_cb(btn, set_temp_via_preset, LV_EVENT_CLICKED, reinterpret_cast<void*>(TARGET_HOTEND_CONFIG_1 + buttonIter + tempIter * 3));
            lv_obj_set_flex_grow(btn, 1);
            lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

            label = lv_label_create(btn);
            lv_label_set_text(label, "???");
            lv_obj_center(label);
            lv_obj_add_event_cb(label, update_temp_preset_label, LV_EVENT_MSG_RECEIVED, reinterpret_cast<void*>(TARGET_HOTEND_CONFIG_1 + buttonIter + tempIter * 3));
            lv_msg_subscribe_obj(DATA_PRINTER_TEMP_PRESET, label, NULL);
        }

        lv_obj_t * btn = lv_btn_create(button_temp_rows[tempIter]);
        lv_obj_add_event_cb(btn, (tempIter == 0) ? show_keyboard_with_hotend : show_keyboard_with_bed, LV_EVENT_CLICKED, panel);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        label = lv_label_create(btn);
        lv_label_set_text(label, "Set");
        lv_obj_center(label);
    }

    lv_obj_t * bottom_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(bottom_panel, element_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(bottom_panel, LV_ALIGN_BOTTOM_MID, 0, -1 * CYD_SCREEN_GAP_PX);
    lv_layout_flex_row(bottom_panel, LV_FLEX_ALIGN_SPACE_EVENLY);

    lv_obj_t * one_above_bottom_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(one_above_bottom_panel, element_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(one_above_bottom_panel, LV_ALIGN_BOTTOM_MID, 0, -1 * CYD_SCREEN_MIN_BUTTON_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    lv_layout_flex_row(one_above_bottom_panel, LV_FLEX_ALIGN_SPACE_EVENLY);

    lv_obj_t * btn = lv_btn_create(bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, btn_extrude, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_DOWN " Extrude");
    lv_obj_center(label);

    btn = lv_btn_create(one_above_bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, btn_retract, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_UP " Retract");
    lv_obj_center(label);

    btn = lv_btn_create(bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, cooldown_temp, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Cooldown");
    lv_obj_center(label);

    btn = lv_btn_create(one_above_bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, btn_toggleable_edit, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Edit Presets");
    lv_obj_center(label);

    lv_msg_send(DATA_PRINTER_TEMP_PRESET, &printer);
}