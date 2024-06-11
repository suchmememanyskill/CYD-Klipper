#include "main_ui.h"
#include "../core/data_setup.h"
#include "../conf/global_config.h"
#include "../core/screen_driver.h"
#include "lvgl.h"
#include "nav_buttons.h"
#include "ui_utils.h"
#include "panels/panel.h"
#include "../core/macros_query.h"
#include "../core/lv_setup.h"
#include "switch_printer.h"
#include "macros.h"

void check_if_screen_needs_to_be_disabled(){
    if (global_config.on_during_print && printer.state == PRINTER_STATE_PRINTING){
        screen_timer_wake();
        screen_timer_stop();
    }
    else {
        screen_timer_start();
    }  
}

static void on_state_change(void * s, lv_msg_t * m){
    check_if_screen_needs_to_be_disabled();
    
    if (printer.state == PRINTER_STATE_OFFLINE){
        nav_buttons_setup(PANEL_CONNECTING);
    }
    else if (printer.state == PRINTER_STATE_ERROR){
        nav_buttons_setup(PANEL_ERROR);
    }
    else if (printer.state == PRINTER_STATE_IDLE) {
        nav_buttons_setup(PANEL_FILES);
    }
    else {
        nav_buttons_setup(PANEL_PROGRESS);
    }
}

void main_ui_setup(){
    lv_msg_subscribe(DATA_PRINTER_STATE, on_state_change, NULL);
    on_state_change(NULL, NULL);
}