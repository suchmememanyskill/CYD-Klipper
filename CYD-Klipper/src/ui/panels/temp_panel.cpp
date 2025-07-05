#include "lvgl.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include "../ui_utils.h"
#include "../../core/printer_integration.hpp"
#include "../../core/current_printer.h"

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


static bool temp_edit_mode = false;
lv_obj_t* root_panel;

static void update_printer_data_hotend_temp(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char hotend_buff[40];
    sprintf(hotend_buff, "Hotend: %.0f C (Target: %.0f C)", 
        get_current_printer_data()->temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexNozzle1], 
        get_current_printer_data()->target_temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexNozzle1]);
    lv_label_set_text(label, hotend_buff);
}

static void update_printer_data_bed_temp(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char bed_buff[40];
    sprintf(bed_buff, "Bed: %.0f C (Target: %.0f C)", 
        get_current_printer_data()->temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexBed], 
        get_current_printer_data()->target_temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexBed]);
    lv_label_set_text(label, bed_buff);
}

static short get_temp_preset(int target){
    switch (target){
        case TARGET_HOTEND_CONFIG_1:
            return get_current_printer()->printer_config->hotend_presets[0];
        case TARGET_HOTEND_CONFIG_2:
            return get_current_printer()->printer_config->hotend_presets[1];
        case TARGET_HOTEND_CONFIG_3:
            return get_current_printer()->printer_config->hotend_presets[2];
        case TARGET_BED_CONFIG_1:
            return get_current_printer()->printer_config->bed_presets[0];
        case TARGET_BED_CONFIG_2:
            return get_current_printer()->printer_config->bed_presets[1];
        case TARGET_BED_CONFIG_3:
            return get_current_printer()->printer_config->bed_presets[2];
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
    write_global_config();
    lv_msg_send(DATA_PRINTER_TEMP_PRESET, get_current_printer());
}

static void keyboard_callback(lv_event_t * e){
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = (lv_obj_t *)lv_event_get_user_data(e);

    const char * text = lv_textarea_get_text(ta);

    int temp = atoi(text);
    if (temp < 0 || temp > 500){
        return;
    }
        
    switch (keyboard_target){
        case TARGET_HOTEND:
            current_printer_set_target_temperature(PrinterTemperatureDevice::PrinterTemperatureDeviceNozzle1, temp);
            break;
        case TARGET_BED:
            current_printer_set_target_temperature(PrinterTemperatureDevice::PrinterTemperatureDeviceBed, temp);
            break;
        case TARGET_HOTEND_CONFIG_1:
            get_current_printer()->printer_config->hotend_presets[0] = temp;
            UpdateConfig();
            break;
        case TARGET_HOTEND_CONFIG_2:
            get_current_printer()->printer_config->hotend_presets[1] = temp;
            UpdateConfig();
            break;
        case TARGET_HOTEND_CONFIG_3:
            get_current_printer()->printer_config->hotend_presets[2] = temp;
            UpdateConfig();
            break;
        case TARGET_BED_CONFIG_1:
            get_current_printer()->printer_config->bed_presets[0] = temp;
            UpdateConfig();
            break;
        case TARGET_BED_CONFIG_2:
            get_current_printer()->printer_config->bed_presets[1] = temp;
            UpdateConfig();
            break;
        case TARGET_BED_CONFIG_3:
            get_current_printer()->printer_config->bed_presets[2] = temp;
            UpdateConfig();
            break;
    }
}

static void show_keyboard_with_hotend(lv_event_t * e){
    keyboard_target = TARGET_HOTEND;
    lv_create_keyboard_text_entry(keyboard_callback, "Set Hotend Temp");
}

static void show_keyboard_with_bed(lv_event_t * e){
    keyboard_target = TARGET_BED;
    lv_create_keyboard_text_entry(keyboard_callback, "Set Bed Temp");
}

static void cooldown_temp(lv_event_t * e){
    if (get_current_printer_data()->state == PrinterState::PrinterStatePrinting){
        return;
    }
    
    current_printer_execute_feature(PrinterFeatures::PrinterFeatureCooldown);
}

static void btn_extrude(lv_event_t * e){
    if (get_current_printer_data()->state == PrinterState::PrinterStatePrinting){
        return;
    }

    current_printer_execute_feature(PrinterFeatures::PrinterFeatureExtrude);
}

static void set_temp_via_preset(lv_event_t * e){
    int target = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    int value = get_temp_preset(target);

    if (temp_edit_mode) {
        keyboard_target = (temp_target)target;
        lv_create_keyboard_text_entry(keyboard_callback, "Set Preset Temp");
        return;
    }

    current_printer_set_target_temperature(target <= TARGET_HOTEND_CONFIG_3
        ? PrinterTemperatureDevice::PrinterTemperatureDeviceNozzle1
        : PrinterTemperatureDevice::PrinterTemperatureDeviceBed
        , value);
}

static void btn_toggleable_edit(lv_event_t * e){
    temp_edit_mode = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
}

static void btn_retract(lv_event_t * e){
    if (get_current_printer_data()->state == PrinterState::PrinterStatePrinting){
        return;
    }

    current_printer_execute_feature(PrinterFeatures::PrinterFeatureRetract);
}

static void set_chart_range(lv_event_t * e) {
    lv_obj_t * chart_obj = lv_event_get_target(e);
    lv_chart_t * chart = (lv_chart_t *)chart_obj;
    int max_temp = 0;
    lv_chart_series_t * prev = NULL;
    
    do {
        prev = lv_chart_get_series_next(chart_obj, prev);

        if (prev != NULL)
            for (int i = 0; i < chart->point_cnt; i++)
                if (prev->y_points[i] > max_temp)
                    max_temp = prev->y_points[i];
        
    } while (prev != NULL);

    int range = ((max_temp + 49) / 50) * 50;

    if (range < 100)
        range = 100;

    lv_chart_set_range(chart_obj, LV_CHART_AXIS_PRIMARY_Y, 0, range);
}

static void set_hotend_temp_chart(lv_event_t * e){
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, get_current_printer_data()->temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexNozzle1]);
}

static void set_hotend_target_temp_chart(lv_event_t * e){
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, get_current_printer_data()->target_temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexNozzle1]);
}

static void set_bed_temp_chart(lv_event_t * e){
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, get_current_printer_data()->temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexBed]);
}

static void set_bed_target_temp_chart(lv_event_t * e){
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, get_current_printer_data()->target_temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexBed]);
}

void create_charts(lv_obj_t * root)
{
    const auto element_width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;
    lv_obj_t * chart = lv_chart_create(root);
    lv_obj_set_size(chart, element_width - CYD_SCREEN_MIN_BUTTON_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * 3);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 120);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, CYD_SCREEN_GAP_PX / 2, CYD_SCREEN_GAP_PX / 4, 4, 3, true, CYD_SCREEN_MIN_BUTTON_WIDTH_PX);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    lv_chart_series_t * ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser1, get_current_printer_data()->target_temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexNozzle1]);
    lv_chart_series_t * ser2 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser2, get_current_printer_data()->temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexNozzle1]);
    lv_chart_series_t * ser3 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_TEAL), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser3, get_current_printer_data()->target_temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexBed]);
    lv_chart_series_t * ser4 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser4, get_current_printer_data()->temperatures[PrinterTemperatureDeviceIndex::PrinterTemperatureDeviceIndexBed]);

    lv_obj_add_event_cb(chart, set_hotend_target_temp_chart, LV_EVENT_MSG_RECEIVED, ser1);
    lv_obj_add_event_cb(chart, set_hotend_temp_chart, LV_EVENT_MSG_RECEIVED, ser2);
    lv_obj_add_event_cb(chart, set_bed_target_temp_chart, LV_EVENT_MSG_RECEIVED, ser3);
    lv_obj_add_event_cb(chart, set_bed_temp_chart, LV_EVENT_MSG_RECEIVED, ser4);
    lv_obj_add_event_cb(chart, set_chart_range, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, chart, NULL);
}

void create_temp_buttons(lv_obj_t * root, lv_obj_t * panel)
{
    const auto element_width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;
    lv_obj_t * temp_rows[2] = {0};
    lv_obj_t * button_temp_rows[2] = {0};

    for (int tempIter = 0; tempIter < 2; tempIter++){
        temp_rows[tempIter] = lv_create_empty_panel(root);
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
}

void temp_panel_init(lv_obj_t * panel){
    const auto element_width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;
    root_panel = panel;
    temp_edit_mode = false;

    lv_obj_t * root_temp_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(root_temp_panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_align(root_temp_panel, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_pad_all(root_temp_panel, CYD_SCREEN_GAP_PX, 0);
    lv_layout_flex_column(root_temp_panel);
    lv_obj_set_flex_align(root_temp_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(root_temp_panel, LV_SCROLLBAR_MODE_OFF);

    #ifndef CYD_SCREEN_NO_TEMP_SCROLL
        create_charts(root_temp_panel);

        lv_obj_t * single_screen_panel = lv_create_empty_panel(root_temp_panel);
        lv_obj_set_size(single_screen_panel, element_width, CYD_SCREEN_PANEL_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2 - CYD_SCREEN_GAP_PX / 2);
        lv_layout_flex_column(single_screen_panel);
    #else
        lv_obj_clear_flag(root_temp_panel, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_t * single_screen_panel = root_temp_panel;
    #endif
    
    create_temp_buttons(single_screen_panel, panel);

    #ifdef CYD_SCREEN_NO_TEMP_SCROLL
        create_charts(single_screen_panel);
    #else
        lv_obj_t * gap = lv_create_empty_panel(single_screen_panel);
        lv_obj_set_flex_grow(gap, 1);
    #endif

    lv_obj_t * one_above_bottom_panel = lv_create_empty_panel(single_screen_panel);
    lv_obj_set_size(one_above_bottom_panel, element_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(one_above_bottom_panel, LV_FLEX_ALIGN_SPACE_EVENLY);

    lv_obj_t * bottom_panel = lv_create_empty_panel(single_screen_panel);
    lv_obj_set_size(bottom_panel, element_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_layout_flex_row(bottom_panel, LV_FLEX_ALIGN_SPACE_EVENLY);

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

    lv_obj_scroll_to_y(root_temp_panel, 9999, LV_ANIM_OFF);
    lv_msg_send(DATA_PRINTER_TEMP_PRESET, get_current_printer());
}