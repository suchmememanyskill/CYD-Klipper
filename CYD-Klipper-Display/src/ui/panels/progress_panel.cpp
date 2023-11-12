#include "panel.h"
#include "../../core/data_setup.h"
#include <stdio.h>

char time_buffer[12];

char* time_display(unsigned long time){
    unsigned long hours = time / 3600;
    unsigned long minutes = (time % 3600) / 60;
    unsigned long seconds = (time % 3600) % 60;
    sprintf(time_buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return time_buffer;
}

static void progress_bar_update(lv_event_t* e){
    lv_obj_t * bar = lv_event_get_target(e);
    lv_bar_set_value(bar, printer.print_progress * 100, LV_ANIM_ON);
}

static void update_printer_data_elapsed_time(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, time_display(printer.elapsed_time_s));
}

static void update_printer_data_remaining_time(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, time_display(printer.remaining_time_s));
}

static void update_printer_data_percentage(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char percentage_buffer[12];
    sprintf(percentage_buffer, "%.2f%%", printer.print_progress * 100);
    lv_label_set_text(label, percentage_buffer);
}

static void btn_click_stop(lv_event_t * e){
    send_gcode(true, "CANCEL_PRINT");
}

static void btn_click_pause(lv_event_t * e){
    send_gcode(true, "PAUSE");
}

static void btn_click_resume(lv_event_t * e){
    send_gcode(true, "RESUME");
}

void progress_panel_init(lv_obj_t* panel){
    auto panel_width = TFT_HEIGHT - 40;
    auto panel_width_margin = panel_width - 30;

    // Filename
    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, printer.print_filename);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -40);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, panel_width_margin);
    
    // Progress Bar
    lv_obj_t * bar = lv_bar_create(panel);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(bar, panel_width_margin, 20);
    lv_obj_add_event_cb(bar, progress_bar_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, bar, NULL);

    // Elapsed Time
    label = lv_label_create(panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 10, 20);
    lv_obj_add_event_cb(label, update_printer_data_elapsed_time, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Remaining Time
    label = lv_label_create(panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, -10, 20);
    lv_obj_add_event_cb(label, update_printer_data_remaining_time, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Percentage
    label = lv_label_create(panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(label, update_printer_data_percentage, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Stop Button
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_add_event_cb(btn, btn_click_stop, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_STOP);
    lv_obj_center(label);

    // Resume Button
    if (printer.state == PRINTER_STATE_PAUSED){
        btn = lv_btn_create(panel);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -60, -10);
        lv_obj_set_size(btn, 40, 40);
        lv_obj_add_event_cb(btn, btn_click_resume, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PLAY);
        lv_obj_center(label);
    }
    // Pause Button
    else {
        btn = lv_btn_create(panel);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -60, -10);
        lv_obj_set_size(btn, 40, 40);
        lv_obj_add_event_cb(btn, btn_click_pause, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PAUSE);
        lv_obj_center(label);
    }

    lv_msg_send(DATA_PRINTER_DATA, &printer);
}