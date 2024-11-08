#include "panel.h"
#include <stdio.h>
#include "../ui_utils.h"
#include "../../core/printer_integration.hpp"
#include "../../core/current_printer.h"

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
    lv_bar_set_value(bar,  get_current_printer_data()->print_progress * 100, LV_ANIM_ON);
}

static void update_printer_data_elapsed_time(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, time_display(get_current_printer_data()->elapsed_time_s));
}

static void update_printer_data_remaining_time(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, time_display(get_current_printer_data()->remaining_time_s));
}

static void update_printer_data_stats(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char buff[256] = {0};

    switch (get_current_printer()->printer_config->show_stats_on_progress_panel)
    {
        case SHOW_STATS_ON_PROGRESS_PANEL_LAYER:
            sprintf(buff, "Layer %d of %d", get_current_printer_data()->current_layer, get_current_printer_data()->total_layers);
            break;
        case SHOW_STATS_ON_PROGRESS_PANEL_PARTIAL:
            sprintf(buff, "Position: X%.2f Y%.2f\nFeedrate: %d mm/s\nFilament Used: %.2f m\nLayer %d of %d", 
            get_current_printer_data()->position[0], get_current_printer_data()->position[1], get_current_printer_data()->feedrate_mm_per_s, get_current_printer_data()->filament_used_mm / 1000, get_current_printer_data()->current_layer, get_current_printer_data()->total_layers);
            break;
        case SHOW_STATS_ON_PROGRESS_PANEL_ALL:
            sprintf(buff, "Pressure Advance: %.3f (%.2fs)\nPosition: X%.2f Y%.2f Z%.2f\nFeedrate: %d mm/s\nFilament Used: %.2f m\nFan: %.0f%%\nSpeed: %.0f%%\nFlow: %.0f%%\nLayer %d of %d", 
            get_current_printer_data()->pressure_advance, get_current_printer_data()->smooth_time, get_current_printer_data()->position[0], get_current_printer_data()->position[1], get_current_printer_data()->position[2], get_current_printer_data()->feedrate_mm_per_s, get_current_printer_data()->filament_used_mm / 1000, get_current_printer_data()->fan_speed * 100, get_current_printer_data()->speed_mult * 100, get_current_printer_data()->extrude_mult * 100, get_current_printer_data()->current_layer, get_current_printer_data()->total_layers);
            break;
    }

    lv_label_set_text(label, buff);
}

static void update_printer_data_percentage(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char percentage_buffer[12];
    sprintf(percentage_buffer, "%.2f%%", get_current_printer_data()->print_progress * 100);
    lv_label_set_text(label, percentage_buffer);
}

static void btn_click_stop(lv_event_t * e){
    current_printer_execute_feature(PrinterFeatures::PrinterFeatureStop);
}

static void btn_click_pause(lv_event_t * e){
    current_printer_execute_feature(PrinterFeatures::PrinterFeaturePause);
}

static void btn_click_resume(lv_event_t * e){
    current_printer_execute_feature(PrinterFeatures::PrinterFeatureResume);
}

static void btn_click_estop(lv_event_t * e){
    current_printer_execute_feature(PrinterFeatures::PrinterFeatureEmergencyStop);
}

void progress_panel_init(lv_obj_t* panel){
    auto panel_width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3;
    const auto button_size_mult = 1.3f;

    // Emergency Stop
    if (global_config.show_estop){
        lv_obj_t * btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_click_estop, LV_EVENT_CLICKED, NULL);
        
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_GAP_PX);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), LV_PART_MAIN);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_POWER " EMERGENCY STOP");
        lv_obj_center(label);
    }

    lv_obj_t * center_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(center_panel, panel_width, LV_SIZE_CONTENT);
    lv_layout_flex_column(center_panel);

    // Only align progress bar to top mid if necessary to make room for all extras
    if (get_current_printer()->printer_config->show_stats_on_progress_panel == SHOW_STATS_ON_PROGRESS_PANEL_ALL && CYD_SCREEN_HEIGHT_PX <= 320)
    {
        lv_obj_align(center_panel, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX+(3 * CYD_SCREEN_GAP_PX));
    }
    else 
    {
        lv_obj_align(center_panel, LV_ALIGN_CENTER, 0, 0);
    }

    // Filename
    lv_obj_t * label = lv_label_create(center_panel);
    lv_label_set_text(label, get_current_printer_data()->print_filename);
    if (global_config.full_filenames) lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    else lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, panel_width);
    
    // Progress Bar
    lv_obj_t * bar = lv_bar_create(center_panel);
    lv_obj_set_size(bar, panel_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * 0.75f);
    lv_obj_add_event_cb(bar, progress_bar_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, bar, NULL);

    // Time
    lv_obj_t * time_est_panel = lv_create_empty_panel(center_panel);
    lv_obj_set_size(time_est_panel, panel_width, LV_SIZE_CONTENT);

    // Elapsed Time
    label = lv_label_create(time_est_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(label, update_printer_data_elapsed_time, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Remaining Time
    label = lv_label_create(time_est_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(label, update_printer_data_remaining_time, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Percentage
    label = lv_label_create(time_est_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, update_printer_data_percentage, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Stop Button
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -1 * CYD_SCREEN_GAP_PX, -1 * CYD_SCREEN_GAP_PX);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * button_size_mult);
    lv_obj_add_event_cb(btn, btn_click_stop, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_STOP);
    lv_obj_center(label);

    // Resume Button
    if (get_current_printer_data()->state == PrinterState::PrinterStatePaused){
        btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_click_resume, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PLAY);
        lv_obj_center(label);
    }
    // Pause Button
    else {
        btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_click_pause, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PAUSE);
        lv_obj_center(label);
    }

    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -2 * CYD_SCREEN_GAP_PX - CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, -1 * CYD_SCREEN_GAP_PX);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * button_size_mult);

    if (get_current_printer()->printer_config->show_stats_on_progress_panel > SHOW_STATS_ON_PROGRESS_PANEL_NONE)
    {
        label = lv_label_create(panel);
        lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, CYD_SCREEN_GAP_PX, -1 * CYD_SCREEN_GAP_PX);
        lv_obj_set_style_text_font(label, &CYD_SCREEN_FONT_SMALL, 0);
        lv_obj_add_event_cb(label, update_printer_data_stats, LV_EVENT_MSG_RECEIVED, NULL);
        lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    }
}