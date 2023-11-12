#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"
#include <HardwareSerial.h>

// False: Hotend, True: Bed
static bool hotend_or_bed = true;
static char hotend_buff[40];
static char bed_buff[40];

static void update_printer_data_hotend_temp(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(hotend_buff, "Hotend: %.0f C\nTarget: %.0f C", printer.extruder_temp, printer.extruder_target_temp);
    lv_label_set_text(label, hotend_buff);
}

static void update_printer_data_bed_temp(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(bed_buff, "Bed: %.0f C\nTarget: %.0f C", printer.bed_temp, printer.bed_target_temp);
    lv_label_set_text(label, bed_buff);
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

        Serial.printf("%d %s %d\n", hotend_or_bed, text, temp);
        char gcode[64];
        const char* space = "%20";

        if (hotend_or_bed){
            sprintf(gcode, "M140%sS%d", space, temp);
        } else {
            sprintf(gcode, "M104%sS%d", space, temp);
        }

        send_gcode(true, gcode);
    }

    if(code == LV_EVENT_DEFOCUSED || code == LV_EVENT_CANCEL || code == LV_EVENT_READY) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_del(kb);
        lv_obj_del(ta);
    }
}

static void show_keyboard(lv_event_t * e){
    lv_obj_t * panel = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t * keyboard = lv_keyboard_create(panel);
    lv_obj_t * ta = lv_textarea_create(panel);
    lv_obj_set_size(ta, 100, 30);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 40);
    lv_textarea_set_max_length(ta, 3);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, "");
    lv_obj_add_event_cb(ta, keyboard_callback, LV_EVENT_ALL, keyboard);

    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, ta);
}

static void show_keyboard_with_hotend(lv_event_t * e){
    hotend_or_bed = false;
    show_keyboard(e);
}

static void show_keyboard_with_bed(lv_event_t * e){
    hotend_or_bed = true;
    show_keyboard(e);
}

static void cooldown_temp(lv_event_t * e){
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }
    
    send_gcode(true, "M104%20S0");
    send_gcode(true, "M140%20S0");
}

static void btn_extrude(lv_event_t * e){
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }

    send_gcode(true, "M83");
    send_gcode(true, "G1%20E25%20F300");
}

static void btn_retract(lv_event_t * e){
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }

    send_gcode(true, "M83");
    send_gcode(true, "G1%20E-25%20F300");
}

void temp_panel_init(lv_obj_t* panel){
    auto panel_width = TFT_HEIGHT - 40;
    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, "Hotend");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(label, update_printer_data_hotend_temp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, label, NULL);

    label = lv_label_create(panel);
    lv_label_set_text(label, "Bed");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_add_event_cb(label, update_printer_data_bed_temp, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, label, NULL);

    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(btn, show_keyboard_with_hotend, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Set");
    lv_obj_center(label);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -10, 50);
    lv_obj_add_event_cb(btn, show_keyboard_with_bed, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Set");
    lv_obj_center(label);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_add_event_cb(btn, cooldown_temp, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Cooldown");
    lv_obj_center(label);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_add_event_cb(btn, btn_extrude, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, panel_width / 2 - 15, 30);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_DOWN " Extrude");
    lv_obj_center(label);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_add_event_cb(btn, btn_retract, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(btn, panel_width / 2 - 15, 30);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_UP " Retract");
    lv_obj_center(label);

    lv_msg_send(DATA_PRINTER_DATA, &printer);
}