#include "main_ui.h"
#include "../core/data_setup.h"
#include "../conf/global_config.h"
#include "../core/screen_driver.h"
#include "../core/printer_integration.hpp"
#include "lvgl.h"
#include "nav_buttons.h"
#include "ui_utils.h"
#include "panels/panel.h"
#include "../core/lv_setup.h"
#include "switch_printer.h"
#include "macros.h"

void check_if_screen_needs_to_be_disabled(){
    if (global_config.on_during_print && get_current_printer_data()->state == PrinterState::PrinterStatePrinting){
        screen_timer_wake();
        screen_timer_stop();
    }
    else {
        screen_timer_start();
    }  
}

static void on_state_change(void * s, lv_msg_t * m){
    check_if_screen_needs_to_be_disabled();
    
    PrinterData* printer = get_current_printer_data();

    if (printer->state == PrinterState::PrinterStateOffline){
        nav_buttons_setup(PANEL_CONNECTING);
    }
    else if (printer->state == PrinterState::PrinterStateError){
        nav_buttons_setup(PANEL_ERROR);
    }
    else if (printer->state == PrinterState::PrinterStateIdle) {
        nav_buttons_setup(PANEL_FILES);
    }
    else {
        nav_buttons_setup(PANEL_PROGRESS);
    }
}

static void on_popup_message(void * s, lv_msg_t * m)
{
    lv_create_popup_message(get_current_printer_data()->popup_message, 10000);
}

void main_ui_setup(){
    lv_msg_subscribe(DATA_PRINTER_STATE, on_state_change, NULL);
    lv_msg_subscribe(DATA_PRINTER_POPUP, on_popup_message, NULL);
    on_state_change(NULL, NULL);
}