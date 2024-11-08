#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <HTTPClient.h>
#include "core/data_setup.h"
#include "ui_utils.h"
#include "panels/panel.h"
#include "macros.h"
#include "../core/lv_setup.h"
#include "serial/serial_console.h"
#include "../core/klipper/klipper_printer_integration.hpp"
#include "../core/bambu/bambu_printer_integration.hpp"
#include "../core/screen_driver.h"
#include "../core/klipper-serial/serial_klipper_printer_integration.hpp"

void show_ip_entry();
void choose_printer_type();

lv_obj_t * main_label;

/* Create a custom keyboard to allow hostnames or ip addresses (a-z, 0 - 9, and -) */
static const char * kb_map[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_OK, "\n",
    LV_SYMBOL_LEFT, "z", "x", "c", "v", "b", "n", "m", ".", "-", LV_SYMBOL_RIGHT, NULL
};

static const lv_btnmatrix_ctrl_t kb_ctrl[] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 5,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, LV_KEYBOARD_CTRL_BTN_FLAGS | 6
};

static const char * hex_numpad_map[] = {
    "1", "2", "3", "F", LV_SYMBOL_BACKSPACE, "\n",
    "4", "5", "6", "E", LV_SYMBOL_OK, "\n",
    "7", "8", "9", "D", LV_SYMBOL_LEFT, "\n",
    "0", "A", "B", "C", LV_SYMBOL_RIGHT, NULL
};

static const lv_btnmatrix_ctrl_t hex_numpad_ctrl[] = {
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
};

static void btn_switch_printer(lv_event_t *e){
    lv_obj_t *btn = lv_event_get_target(e);
    PrinterConfiguration * config = (PrinterConfiguration*)lv_event_get_user_data(e);
    int index = config - global_config.printer_config;

    global_config_set_printer(index);
    set_color_scheme();
    set_invert_display();
    lv_obj_del(lv_obj_get_parent(lv_obj_get_parent(btn)));
}

long last_request = 0;

void serial_check_connection()
{
    if ((millis() - last_request) < 5000)
    {
        return;
    }

    auto result = connection_test_serial_klipper(&global_config.printer_config[global_config.printer_index]);
    last_request = millis();

    if (result == KlipperConnectionStatus::ConnectOk)
    {
        global_config.printer_config[global_config.printer_index].setup_complete = true;
        strcpy(global_config.printer_config[global_config.printer_index].klipper_host, "Serial");
        write_global_config();
    }
}

void switch_printer_init() {
    lv_obj_t * parent = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_style_bg_opa(parent, LV_OPA_100, 0); 
    lv_obj_align(parent, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(parent, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(parent);

    lv_obj_set_size(lv_create_empty_panel(parent), 0, 0);

    auto width = CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, parent);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Close");
    lv_obj_center(label);

    for (int i = 0; i < PRINTER_CONFIG_COUNT; i++){
        PrinterConfiguration * config = &global_config.printer_config[i];
        const char* printer_name = (config->printer_name[0] == 0) ? config->klipper_host : config->printer_name;

        if (i == global_config.printer_index && config->setup_complete)
        {
            lv_create_custom_menu_label(printer_name, parent, "Active");
            continue;
        }

        if (config->setup_complete) {
            lv_create_custom_menu_button(printer_name, parent, btn_switch_printer, "Switch", config);
        }
    }
}

static void show_switch_printer_screen(lv_event_t * e){
    switch_printer_init();
}

static void host_update(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_target(e);
    const char* text = lv_textarea_get_text(ta);
    strcpy(global_config.printer_config[global_config.printer_index].klipper_host, text);
    global_config.printer_config[global_config.printer_index].ip_configured = text[0] != '\0';
}

static void port_update(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_target(e);
    const char* text = lv_textarea_get_text(ta);
    if (text[0] != '\0')
    {
        global_config.printer_config[global_config.printer_index].klipper_port = atoi(text);
    }
}

static void auth_update(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_target(e);
    const char* text = lv_textarea_get_text(ta);
    strcpy(global_config.printer_config[global_config.printer_index].klipper_auth, text);
    global_config.printer_config[global_config.printer_index].auth_configured = text[0] != '\0';
}

static void return_to_choose_printer_type(lv_event_t * e)
{
    choose_printer_type();
}

static void keyboard_event_ip_entry(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if ((code == LV_EVENT_FOCUSED || code == LV_EVENT_DEFOCUSED) && ta != NULL)
    {
        // make sure we alter the keymap before taking actions that might
        // destroy the keyboard
        if (lv_obj_has_flag(ta, LV_OBJ_FLAG_USER_1))
        {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_1);
        }
        else if (lv_obj_has_flag(ta, LV_OBJ_FLAG_USER_2))
        {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_USER_2);
        }
        else
        {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
        }
    }

    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_READY) 
    {
        PrinterType type = global_config.printer_config[global_config.printer_index].printer_type;

        if (type == PrinterType::PrinterTypeKlipper)
        {
            KlipperConnectionStatus klipper_status = connection_test_klipper(&global_config.printer_config[global_config.printer_index]);
            if (klipper_status == KlipperConnectionStatus::ConnectOk)
            {
                global_config.printer_config[global_config.printer_index].setup_complete = true;
                write_global_config();
            }
            else if (klipper_status == KlipperConnectionStatus::ConnectAuthRequired)
            {
                lv_label_set_text(main_label, "Incorrect authorisation");
            }
            else
            {
                lv_label_set_text(main_label, "Failed to connect");
            }
        }
        else if (type == PrinterType::PrinterTypeBambuLocal)
        {
            BambuConnectionStatus bambu_status = connection_test_bambu(&global_config.printer_config[global_config.printer_index]);
            if (bambu_status == BambuConnectionStatus::BambuConnectOk)
            {
                global_config.printer_config[global_config.printer_index].setup_complete = true;
                write_global_config();
            }
            else if (bambu_status == BambuConnectionStatus::BambuConnectSNFail)
            {
                lv_label_set_text(main_label, "Incorrect serial number");
            }
            else
            {
                lv_label_set_text(main_label, "Incorrect IP/Access code");
            }
        }
    }
    else
    {
        return;
    }
}

void show_ip_entry()
{
    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(root);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * top_root = lv_create_empty_panel(root);
    lv_obj_set_width(top_root, CYD_SCREEN_WIDTH_PX);
    lv_layout_flex_column(top_root);
    lv_obj_set_flex_grow(top_root, 1);
    lv_obj_set_style_pad_all(top_root, CYD_SCREEN_GAP_PX, 0);
    lv_obj_clear_flag(top_root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * button_row = lv_create_empty_panel(top_root);
    lv_obj_set_size(button_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, LV_SIZE_CONTENT);
    lv_layout_flex_row(button_row);

    lv_obj_t * button_back = lv_btn_create(button_row);
    lv_obj_set_height(button_back, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX / 2);
    lv_obj_set_flex_grow(button_back, 1);
    lv_obj_add_event_cb(button_back, return_to_choose_printer_type, LV_EVENT_CLICKED, NULL);

    
    lv_obj_t * label = lv_label_create(button_back);
    lv_label_set_text(label, LV_SYMBOL_LEFT);
    lv_obj_center(label);

    main_label = lv_label_create(button_row);

    if (global_config.multi_printer_mode)
    {
        lv_obj_t * button_switch_printer = lv_btn_create(button_row);
        lv_obj_set_height(button_switch_printer, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX / 2);
        lv_obj_set_flex_grow(button_switch_printer, 1);
        lv_obj_add_event_cb(button_switch_printer, show_switch_printer_screen, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(button_switch_printer);
        lv_label_set_text(label, LV_SYMBOL_HOME);
        lv_obj_center(label);
    }

    lv_obj_t * ip_row = lv_create_empty_panel(top_root);
    lv_obj_set_size(ip_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, LV_SIZE_CONTENT);
    lv_layout_flex_row(ip_row);

    lv_obj_t * host_entry = lv_textarea_create(ip_row);
    lv_textarea_set_one_line(host_entry, true);
    lv_obj_add_flag(host_entry, LV_OBJ_FLAG_USER_1);
    lv_textarea_set_max_length(host_entry, 64);
    lv_obj_set_flex_grow(host_entry, 2);

    lv_obj_t * port_entry = lv_textarea_create(ip_row);
    lv_textarea_set_one_line(port_entry, true);
    lv_obj_set_flex_grow(port_entry, 1);

    lv_obj_t * auth_row = lv_create_empty_panel(top_root);
    lv_obj_set_size(auth_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, LV_SIZE_CONTENT);
    lv_layout_flex_row(auth_row);

    lv_obj_t * auth_entry = lv_textarea_create(auth_row);
    lv_textarea_set_one_line(auth_entry, true);
    lv_obj_add_flag(auth_entry, LV_OBJ_FLAG_USER_2);
    lv_obj_set_flex_grow(auth_entry, 1);
    lv_textarea_set_max_length(auth_entry, 32);

    if (global_config.printer_config[global_config.printer_index].ip_configured)
    {
        char buff[10] = {0};
        sprintf(buff, "%d", global_config.printer_config[global_config.printer_index].klipper_port);
        lv_textarea_set_text(host_entry, global_config.printer_config[global_config.printer_index].klipper_host);
        lv_textarea_set_text(port_entry, buff);
    }
    else 
    {
        lv_textarea_set_text(host_entry, "");

        if (global_config.printer_config[global_config.printer_index].printer_type == PrinterType::PrinterTypeBambuLocal)
        {
            lv_textarea_set_text(port_entry, "");
            global_config.printer_config[global_config.printer_index].klipper_port = 0;
        }
        else
        {
            lv_textarea_set_text(port_entry, "80");
            global_config.printer_config[global_config.printer_index].klipper_port = 80;
        }
        
        global_config.printer_config[global_config.printer_index].klipper_host[0] = '\0';
        
    }

    if (global_config.printer_config[global_config.printer_index].auth_configured)
    {
        lv_textarea_set_text(auth_entry, global_config.printer_config[global_config.printer_index].klipper_auth);
    }
    else
    {
        lv_textarea_set_text(auth_entry, "");
        global_config.printer_config[global_config.printer_index].klipper_auth[0] = '\0';
    }

    lv_obj_add_event_cb(host_entry, host_update, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(port_entry, port_update, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(auth_entry, auth_update, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_2, hex_numpad_map, hex_numpad_ctrl);
    lv_obj_add_event_cb(host_entry, keyboard_event_ip_entry, LV_EVENT_ALL, keyboard);
    lv_obj_add_event_cb(port_entry, keyboard_event_ip_entry, LV_EVENT_ALL, keyboard);
    lv_obj_add_event_cb(auth_entry, keyboard_event_ip_entry, LV_EVENT_ALL, keyboard);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_textarea(keyboard, host_entry);

    switch (global_config.printer_config[global_config.printer_index].printer_type)
    {
        case PrinterType::PrinterTypeKlipper:
            lv_label_set_text(main_label, "Klipper Setup");
            lv_textarea_set_max_length(port_entry, 5);
            lv_textarea_set_placeholder_text(host_entry, "Klipper host");
            lv_textarea_set_placeholder_text(port_entry, "Port");
            lv_textarea_set_placeholder_text(auth_entry, "Autorisation key (optional)");
            break;
        case PrinterType::PrinterTypeBambuLocal:
            lv_label_set_text(main_label, "Bambu (Local) Setup");
            lv_obj_set_flex_grow(port_entry, 4);
            lv_obj_set_flex_grow(host_entry, 6);
            lv_textarea_set_max_length(port_entry, 8);
            lv_textarea_set_placeholder_text(host_entry, "Printer IP");
            lv_textarea_set_placeholder_text(port_entry, "Access code");
            lv_textarea_set_placeholder_text(auth_entry, "Printer serial number");
            break;
        case PrinterType::PrinterTypeKlipperSerial:
            lv_label_set_text(main_label, "Klipper (Serial) Setup");
            lv_obj_del(ip_row);
            lv_obj_del(auth_row);
            lv_obj_del(keyboard);

            lv_obj_t * bottom_root = lv_create_empty_panel(top_root);
            lv_obj_set_width(bottom_root, CYD_SCREEN_WIDTH_PX);
            lv_obj_set_flex_grow(bottom_root, 1);
            
            label = lv_label_create(bottom_root);
            lv_obj_center(label);
            lv_label_set_text(label, "Connect CYD-Klipper to a host\nrunning the CYD-Klipper server");
            break;
    }
}

static void printer_type_klipper(lv_event_t * e)
{
    global_config.printer_config[global_config.printer_index].printer_type = PrinterType::PrinterTypeKlipper;
    show_ip_entry();
}

static void printer_type_bambu_local(lv_event_t * e)
{
    global_config.printer_config[global_config.printer_index].printer_type = PrinterType::PrinterTypeBambuLocal;
    show_ip_entry();
}

static void printer_type_serial_klipper(lv_event_t * e)
{
    global_config.printer_config[global_config.printer_index].printer_type = PrinterType::PrinterTypeKlipperSerial;
    show_ip_entry();
}

void choose_printer_type()
{
    lv_obj_clean(lv_scr_act());
    global_config.printer_config[global_config.printer_index].ip_configured = false;
    global_config.printer_config[global_config.printer_index].auth_configured = false;

    lv_obj_t * root = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(root);
    lv_obj_set_flex_grow(root, 1);
    lv_obj_set_style_pad_all(root, CYD_SCREEN_GAP_PX, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(root);
    lv_label_set_text(label, "Choose printer type");

    lv_obj_t * btn = lv_btn_create(root);
    lv_obj_set_size(btn, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, printer_type_klipper, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Klipper");
    lv_obj_center(label);

    btn = lv_btn_create(root);
    lv_obj_set_size(btn, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, printer_type_serial_klipper, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Klipper (Serial)");
    lv_obj_center(label);

    btn = lv_btn_create(root);
    lv_obj_set_size(btn, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, printer_type_bambu_local, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Bambu (Local)");
    lv_obj_center(label);
}

void ip_init(){
    if (!global_config.printer_config[global_config.printer_index].setup_complete)
    {
        if (global_config.printer_config[global_config.printer_index].printer_type == PrinterType::PrinterTypeNone)
        {
            choose_printer_type();
        }
        else
        {
            show_ip_entry();
        }
    }
    
    while (!global_config.printer_config[global_config.printer_index].setup_complete)
    {
        if (global_config.printer_config[global_config.printer_index].printer_type == PrinterType::PrinterTypeKlipperSerial)
        {
            serial_check_connection();
        }

        lv_handler();
        serial_console::run();
    }
}