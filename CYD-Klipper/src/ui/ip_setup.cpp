#include "ip_setup.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include <HTTPClient.h>
#include "core/data_setup.h"
#include "ui_utils.h"
#include "panels/panel.h"
#include "switch_printer.h"
#include "macros.h"
#include "../core/lv_setup.h"
#include "serial/serial_console.h"
#include "../core/klipper/klipper_printer_integration.hpp"

lv_obj_t * hostEntry;
lv_obj_t * portEntry;
lv_obj_t * label = NULL;

void show_ip_entry();
void show_auth_entry();

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
    "1", "2", "3", "f", LV_SYMBOL_BACKSPACE, "\n",
    "4", "5", "6", "e", LV_SYMBOL_OK, "\n",
    "7", "8", "9", "d", LV_SYMBOL_LEFT, "\n",
    "0", "a", "b", "c", LV_SYMBOL_RIGHT, NULL
};

static const lv_btnmatrix_ctrl_t hex_numpad_ctrl[] = {
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
};

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
        strcpy(global_config.printer_config[global_config.printer_index].klipper_host, lv_textarea_get_text(hostEntry));
        global_config.printer_config[global_config.printer_index].klipper_port = atoi(lv_textarea_get_text(portEntry));

        ConnectionStatus status = connection_test_klipper(&global_config.printer_config[global_config.printer_index]);
        if (status == ConnectionStatus::ConnectOk)
        {
            global_config.printer_config[global_config.printer_index].ip_configured = true;
            global_config.printer_config[global_config.printer_index].setup_complete = true;
            write_global_config();
        }
        else if (status == ConnectionStatus::ConnectAuthRequired)
        {
            show_auth_entry();
        }
        else
        {
            lv_label_set_text(label, "Failed to connect");
        }
    }
    else
    {
        return;
    }
}

static void keyboard_event_auth_entry(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_READY) 
    {
        const char * txt = lv_textarea_get_text(ta);
        int len = strlen(txt);
        if (len > 0)
        {
            global_config.printer_config[global_config.printer_index].auth_configured = true;
            strcpy(global_config.printer_config[global_config.printer_index].klipper_auth, txt);

            if (connection_test_klipper(&global_config.printer_config[global_config.printer_index]) == ConnectionStatus::ConnectOk)
            {
                global_config.printer_config[global_config.printer_index].ip_configured = true;
                global_config.printer_config[global_config.printer_index].setup_complete = true;
                write_global_config();
            }
            else 
            {
                lv_label_set_text(label, "Failed to connect");
            }
        }
    }
    else if (code == LV_EVENT_CANCEL)
    {
        show_ip_entry();
    }
}

void show_auth_entry()
{
    global_config.printer_config[global_config.printer_index].klipper_auth[32] = 0;
    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(root);

    lv_obj_t * top_root = lv_create_empty_panel(root);
    lv_obj_set_width(top_root, CYD_SCREEN_WIDTH_PX);
    lv_layout_flex_column(top_root);
    lv_obj_set_flex_grow(top_root, 1);
    lv_obj_set_style_pad_all(top_root, CYD_SCREEN_GAP_PX, 0);

    label = lv_label_create(top_root);
    lv_label_set_text(label, "Enter API Key");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_obj_t * passEntry = lv_textarea_create(top_root);
    lv_textarea_set_max_length(passEntry, 32);
    lv_textarea_set_one_line(passEntry, true);

    if (global_config.printer_config[global_config.printer_index].auth_configured)
        lv_textarea_set_text(passEntry, global_config.printer_config[global_config.printer_index].klipper_auth);
    else
        lv_textarea_set_text(passEntry, "");

    lv_obj_set_width(passEntry, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_add_event_cb(passEntry, keyboard_event_auth_entry, LV_EVENT_ALL, keyboard);
    lv_obj_set_flex_grow(passEntry, 1);
    
    lv_keyboard_set_textarea(keyboard, passEntry);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_2, hex_numpad_map, hex_numpad_ctrl);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_2);
}

void show_ip_entry()
{
    lv_obj_clean(lv_scr_act());

    lv_obj_t * root = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(root, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(root);

    lv_obj_t * top_root = lv_create_empty_panel(root);
    lv_obj_set_width(top_root, CYD_SCREEN_WIDTH_PX);
    lv_layout_flex_column(top_root);
    lv_obj_set_flex_grow(top_root, 1);
    lv_obj_set_style_pad_all(top_root, CYD_SCREEN_GAP_PX, 0);

    label = lv_label_create(top_root);
    lv_label_set_text(label, "Enter Klipper IP/Hostname and Port");
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);

    lv_obj_t * textbow_row = lv_create_empty_panel(top_root);
    lv_obj_set_width(textbow_row, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2);
    lv_obj_set_flex_grow(textbow_row, 1);
    lv_layout_flex_row(textbow_row);

    hostEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(hostEntry, true);
    lv_obj_add_flag(hostEntry, LV_OBJ_FLAG_USER_1);
    lv_textarea_set_max_length(hostEntry, 63);
    lv_textarea_set_text(hostEntry, "");
    lv_obj_set_flex_grow(hostEntry, 3);

    portEntry = lv_textarea_create(textbow_row);
    lv_textarea_set_one_line(portEntry, true);
    lv_textarea_set_max_length(portEntry, 5);
    lv_textarea_set_text(portEntry, "80");
    lv_obj_set_flex_grow(portEntry, 1);

    lv_obj_t * keyboard = lv_keyboard_create(root);
    lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, kb_map, kb_ctrl);
    lv_obj_add_event_cb(hostEntry, keyboard_event_ip_entry, LV_EVENT_ALL, keyboard);
    lv_obj_add_event_cb(portEntry, keyboard_event_ip_entry, LV_EVENT_ALL, keyboard);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
    lv_keyboard_set_textarea(keyboard, hostEntry);

    if (global_config.multi_printer_mode)
    {
        lv_obj_t * btn = draw_switch_printer_button();
        lv_obj_set_parent(btn, textbow_row);
        lv_obj_align(btn, LV_ALIGN_DEFAULT, 0, 0);
    }
}

void ip_init(){
    if (!global_config.printer_config[global_config.printer_index].setup_complete)
    {
        show_ip_entry();
    }
    
    while (!global_config.printer_config[global_config.printer_index].setup_complete)
    {
        lv_handler();
        serial_console::run();
    }
}