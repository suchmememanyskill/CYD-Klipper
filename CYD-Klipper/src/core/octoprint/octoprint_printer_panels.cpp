#include "octoprint_printer_integration.hpp"
#include "lvgl.h"
#include "../../ui/ui_utils.h"
#include <stdio.h>

const char* COMMAND_EXTRUDE_MULT = "{\"command\":\"flowrate\",\"factor\":%d}";

#define OCTO_TIMEOUT_POPUP_MESSAGES 4000

static void set_fan_speed_text(lv_event_t * e) 
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, "Fan");
}

static void set_speed_mult_text(lv_event_t * e) 
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, "Speed");
}

static void set_extruder_mult_text(lv_event_t * e) 
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, "Flowrate");
}

bool get_range(lv_event_t * e, int min, int max, int* out)
{
    char buff[64];
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);
    const char * txt = lv_textarea_get_text(ta);

    if (txt == NULL || *txt == '\0')
    {
        return false;
    }

    int parsed_input = atoi(txt);
    if (parsed_input < min || parsed_input > max)
    {
        sprintf(buff, "Value out of range (%d -> %d)", min, max);
        lv_create_popup_message(buff, OCTO_TIMEOUT_POPUP_MESSAGES);
        return false;
    }

    *out = parsed_input;
    return true;
}

static void set_fan_speed(lv_event_t * e)
{
    int fan_speed = 0;
    if (get_range(e, 0, 100, &fan_speed))
    {
        int actual_fan_speed = fan_speed * 255 / 100;
        char buff[16];
        sprintf(buff, "M106 S%d", actual_fan_speed);
        ((OctoPrinter*)get_current_printer())->send_gcode(buff);
    }
}

static void open_fan_speed_keypad(lv_event_t * e)
{
    lv_create_keyboard_text_entry(set_fan_speed, "New fan speed %", LV_KEYBOARD_MODE_NUMBER);
}

static void set_speed_mult(lv_event_t * e)
{
    int speed_mult = 0;
    if (get_range(e, 50, 300, &speed_mult))
    {
        char buff[16];
        sprintf(buff, "M220 S%d", speed_mult);
        ((OctoPrinter*)get_current_printer())->send_gcode(buff);
    }
}

static void open_speed_mult_keypad(lv_event_t * e)
{
    lv_create_keyboard_text_entry(set_speed_mult, "New speed multiplier %", LV_KEYBOARD_MODE_NUMBER);
}

static void set_extrude_mult(lv_event_t * e)
{
    int extrude_mult = 0;
    if (get_range(e, 75, 125, &extrude_mult))
    {
        char buff[64];
        sprintf(buff, COMMAND_EXTRUDE_MULT, extrude_mult);
        ((OctoPrinter*)get_current_printer())->post_request("/api/printer/tool", buff);
    }
}

static void open_extrude_mult_keypad(lv_event_t * e)
{
    lv_create_keyboard_text_entry(set_extrude_mult, "New extrude multiplier %", LV_KEYBOARD_MODE_NUMBER);
}

static PrinterUiPanel klipper_ui_panels[4] {
    { .set_label = (void*)set_fan_speed_text, .open_panel = (void*)open_fan_speed_keypad },
    { .set_label = (void*)set_speed_mult_text, .open_panel = (void*)open_speed_mult_keypad },
    { .set_label = (void*)set_extruder_mult_text, .open_panel = (void*)open_extrude_mult_keypad },
};

void OctoPrinter::init_ui_panels()
{
    custom_menus_count = 3;
    custom_menus = klipper_ui_panels;
}