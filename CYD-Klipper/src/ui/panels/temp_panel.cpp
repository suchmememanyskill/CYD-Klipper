#include "lvgl.h"
#include "../../core/data_setup.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include "../ui_utils.h"

enum TEMP_TARGET {
    TARGET_HOTEND,
    TARGET_BED,
    TARGET_HOTEND_CONFIG_1,
    TARGET_HOTEND_CONFIG_2,
    TARGET_HOTEND_CONFIG_3,
    TARGET_BED_CONFIG_1,
    TARGET_BED_CONFIG_2,
    TARGET_BED_CONFIG_3,
};

static TEMP_TARGET keyboardTarget;
static char hotendBuff[40];
static char bedBuff[40];
static bool editMode = false;
lv_obj_t* rootPanel;

static void updatePrinterDataHotendTemp(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(hotendBuff, "Hotend: %.0f C (Target: %.0f C)", printer.extruderTemp, printer.extruderTargetTemp);
    lv_label_set_text(label, hotendBuff);
}

static void updatePrinterDataBedTemp(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(bedBuff, "Bed: %.0f C (Target: %.0f C)", printer.bedTemp, printer.bedTargetTemp);
    lv_label_set_text(label, bedBuff);
}

static short getTempPreset(int target) {
    switch (target) {
        case TARGET_HOTEND_CONFIG_1:
            return globalConfig.hotendPresets[0];
        case TARGET_HOTEND_CONFIG_2:
            return globalConfig.hotendPresets[1];
        case TARGET_HOTEND_CONFIG_3:
            return globalConfig.hotendPresets[2];
        case TARGET_BED_CONFIG_1:
            return globalConfig.bedPresets[0];
        case TARGET_BED_CONFIG_2:
            return globalConfig.bedPresets[1];
        case TARGET_BED_CONFIG_3:
            return globalConfig.bedPresets[2];
        default:
            return -1;
    }
}

static void updateTempPresetLabel(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    int target = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    short value = getTempPreset(target);

    String textLabel = String(value) + " C";
    lv_label_set_text(label, textLabel.c_str());
}

void UpdateConfig() {
    WriteGlobalConfig();
    lv_msg_send(DATA_PRINTER_TEMP_PRESET, &printer);
}

static void keyboardCallback(lv_event_t * e) {
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

        switch (keyboardTarget) {
            case TARGET_HOTEND:
                sprintf(gcode, "M104 S%d", temp);
                sendGcode(true, gcode);
                break;
            case TARGET_BED:
                sprintf(gcode, "M140 S%d", temp);
                sendGcode(true, gcode);
                break;
            case TARGET_HOTEND_CONFIG_1:
                globalConfig.hotendPresets[0] = temp;
                UpdateConfig();
                break;
            case TARGET_HOTEND_CONFIG_2:
                globalConfig.hotendPresets[1] = temp;
                UpdateConfig();
                break;
            case TARGET_HOTEND_CONFIG_3:
                globalConfig.hotendPresets[2] = temp;
                UpdateConfig();
                break;
            case TARGET_BED_CONFIG_1:
                globalConfig.bedPresets[0] = temp;
                UpdateConfig();
                break;
            case TARGET_BED_CONFIG_2:
                globalConfig.bedPresets[1] = temp;
                UpdateConfig();
                break;
            case TARGET_BED_CONFIG_3:
                globalConfig.bedPresets[2] = temp;
                UpdateConfig();
                break;
        }
    }

    if(code == LV_EVENT_DEFOCUSED || code == LV_EVENT_CANCEL || code == LV_EVENT_READY) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_del(lv_obj_get_parent(kb));
    }
}

static void showKeyboard(lv_event_t * e) {
    lv_obj_t * parent = CreateEmptyPanel(rootPanel);
    lv_obj_set_style_bg_opa(parent, LV_OPA_50, 0);
    lv_obj_set_size(parent, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    LayoutFlexColumn(parent, LV_FLEX_ALIGN_SPACE_BETWEEN);

    lv_obj_t * emptyPanel = CreateEmptyPanel(parent);
    lv_obj_set_flex_grow(emptyPanel, 1);

    lv_obj_t * ta = lv_textarea_create(parent);
    lv_obj_t * keyboard = lv_keyboard_create(parent);

    lv_obj_set_width(ta, CYD_SCREEN_PANEL_WIDTH_PX / 2);
    lv_textarea_set_max_length(ta, 3);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_text(ta, "");
    lv_obj_add_event_cb(ta, keyboardCallback, LV_EVENT_ALL, keyboard);

    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, ta);
}

static void showKeyboardWithHotend(lv_event_t * e) {
    keyboardTarget = TARGET_HOTEND;
    showKeyboard(e);
}

static void showKeyboardWithBed(lv_event_t * e) {
    keyboardTarget = TARGET_BED;
    showKeyboard(e);
}

static void cooldownTemp(lv_event_t * e) {
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }

    sendGcode(true, "M104 S0");
    sendGcode(true, "M140 S0");
}

static void btnExtrude(lv_event_t * e) {
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }

    sendGcode(true, "M83");
    sendGcode(true, "G1 E25 F300");
}

static void setTempViaPreset(lv_event_t * e) {
    int target = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    int value = getTempPreset(target);

    if (editMode) {
        keyboardTarget = (TEMP_TARGET)target;
        showKeyboard(e);
        return;
    }

    char gcode[64];
    if (target <= TARGET_HOTEND_CONFIG_3)
        sprintf(gcode, "M104 S%d", value);
    else
        sprintf(gcode, "M140 S%d", value);

    sendGcode(true, gcode);
}

static void btnToggleableEdit(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    auto state = lv_obj_get_state(btn);
    editMode = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
}

static void btnRetract(lv_event_t * e) {
    if (printer.state == PRINTER_STATE_PRINTING){
        return;
    }

    sendGcode(true, "M83");
    sendGcode(true, "G1 E-25 F300");
}

static void setChartRange(lv_event_t * e) {
    lv_obj_t * chartObj = lv_event_get_target(e);
    lv_chart_t * chart = (lv_chart_t *)chartObj;
    int maxTemp = 0;
    lv_chart_series_t * prev = NULL;

    do {
        prev = lv_chart_get_series_next(chartObj, prev);

        if (prev != NULL)
            for (int i = 0; i < chart->point_cnt; i++)
                if (prev->y_points[i] > maxTemp)
                    maxTemp = prev->y_points[i];

    } while (prev != NULL);

    int range = ((maxTemp + 49) / 50) * 50;

    if (range < 100)
        range = 100;

    lv_chart_set_range(chartObj, LV_CHART_AXIS_PRIMARY_Y, 0, range);
}

static void setHotendTempChart(lv_event_t * e) {
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, printer.extruderTemp);
}

static void setHotendTargetTempChart(lv_event_t * e) {
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, printer.extruderTargetTemp);
}

static void setBedTempChart(lv_event_t * e) {
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, printer.bedTemp);
}

static void setBedTargetTempChart(lv_event_t * e) {
    lv_obj_t * chart = lv_event_get_target(e);
    lv_chart_series_t * series = (lv_chart_series_t *)lv_event_get_user_data(e);
    lv_chart_set_next_value(chart, series, printer.bedTargetTemp);
}

void tempPanelInit(lv_obj_t * panel) {
    const auto elementWidth = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;
    rootPanel = panel;
    editMode = false;

    lv_obj_t * rootTempPanel = CreateEmptyPanel(panel);
    lv_obj_set_size(rootTempPanel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_align(rootTempPanel, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_pad_all(rootTempPanel, CYD_SCREEN_GAP_PX, 0);
    LayoutFlexColumn(rootTempPanel);
    lv_obj_set_flex_align(rootTempPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(rootTempPanel, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t * chart = lv_chart_create(rootTempPanel);
    lv_obj_set_size(chart, elementWidth - CYD_SCREEN_MIN_BUTTON_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * 3);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, 120);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, CYD_SCREEN_GAP_PX / 2, CYD_SCREEN_GAP_PX / 4, 4, 3, true, CYD_SCREEN_MIN_BUTTON_WIDTH_PX);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    lv_chart_series_t * ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser1, printer.extruderTargetTemp);
    lv_chart_series_t * ser2 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser2, printer.extruderTemp);
    lv_chart_series_t * ser3 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_TEAL), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser3, printer.bedTargetTemp);
    lv_chart_series_t * ser4 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

    lv_chart_series_t * ser4 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser4, printer.bedTemp);

    lv_obj_add_event_cb(chart, setHotendTargetTempChart, LV_EVENT_MSG_RECEIVED, ser1);
    lv_obj_add_event_cb(chart, setHotendTempChart, LV_EVENT_MSG_RECEIVED, ser2);
    lv_obj_add_event_cb(chart, setBedTargetTempChart, LV_EVENT_MSG_RECEIVED, ser3);
    lv_obj_add_event_cb(chart, setBedTempChart, LV_EVENT_MSG_RECEIVED, ser4);
    lv_obj_add_event_cb(chart, setChartRange, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, chart, NULL);

    lv_obj_t * single_screen_panel = CreateEmptyPanel(rootTempPanel);
    lv_obj_set_size(single_screen_panel, elementWidth, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2 - CYD_SCREEN_GAP_PX / 2);
    LayoutFlexColumn(single_screen_panel);

    lv_obj_t * temp_rows[2] = {0};
    lv_obj_t * button_temp_rows[2] = {0};

    for (int tempIter = 0; tempIter < 2; tempIter++){
        temp_rows[tempIter] = CreateEmptyPanel(single_screen_panel);
        LayoutFlexRow(temp_rows[tempIter]);
        lv_obj_set_size(temp_rows[tempIter], elementWidth, LV_SIZE_CONTENT);

        lv_obj_t * label = lv_label_create(temp_rows[tempIter]);
        lv_label_set_text(label, "???");
        lv_obj_add_event_cb(label, (tempIter == 0) ? updatePrinterDataHotendTemp : updatePrinterDataBedTemp, LV_EVENT_MSG_RECEIVED, NULL);
        lv_msg_subscribe_obj(DATA_PRINTER_DATA, label, NULL);
        lv_obj_set_width(label, elementWidth);

        button_temp_rows[tempIter] = CreateEmptyPanel(temp_rows[tempIter]);
        LayoutFlexRow(button_temp_rows[tempIter], LV_FLEX_ALIGN_SPACE_EVENLY);
        lv_obj_set_size(button_temp_rows[tempIter], elementWidth, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        for (int buttonIter = 0; buttonIter < 3; buttonIter++){
            lv_obj_t * btn = lv_btn_create(button_temp_rows[tempIter]);
            lv_obj_add_event_cb(btn, setTempViaPreset, LV_EVENT_CLICKED, reinterpret_cast<void*>(TARGET_HOTEND_CONFIG_1 + buttonIter + tempIter * 3));
            lv_obj_set_flex_grow(btn, 1);
            lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

            label = lv_label_create(btn);
            lv_label_set_text(label, "???");
            lv_obj_center(label);
            lv_obj_add_event_cb(label, updateTempPresetLabel, LV_EVENT_MSG_RECEIVED, reinterpret_cast<void*>(TARGET_HOTEND_CONFIG_1 + buttonIter + tempIter * 3));
            lv_msg_subscribe_obj(DATA_PRINTER_TEMP_PRESET, label, NULL);
        }

        lv_obj_t * btn = lv_btn_create(button_temp_rows[tempIter]);
        lv_obj_add_event_cb(btn, (tempIter == 0) ? showKeyboardWithHotend : showKeyboardWithBed, LV_EVENT_CLICKED, panel);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

        label = lv_label_create(btn);
        lv_label_set_text(label, "Set");
        lv_obj_center(label);
    }

    lv_obj_t * gap = CreateEmptyPanel(single_screen_panel);
    lv_obj_set_flex_grow(gap, 1);

    lv_obj_t * one_above_bottom_panel = CreateEmptyPanel(single_screen_panel);
    lv_obj_set_size(one_above_bottom_panel, elementWidth, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    LayoutFlexRow(one_above_bottom_panel, LV_FLEX_ALIGN_SPACE_EVENLY);

    lv_obj_t * bottom_panel = CreateEmptyPanel(single_screen_panel);
    lv_obj_set_size(bottom_panel, elementWidth, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    LayoutFlexRow(bottom_panel, LV_FLEX_ALIGN_SPACE_EVENLY);

    lv_obj_t * btn = lv_btn_create(bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, btnExtrude, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_DOWN " Extrude");
    lv_obj_center(label);

    btn = lv_btn_create(one_above_bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, btnRetract, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_UP " Retract");
    lv_obj_center(label);

    btn = lv_btn_create(bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, cooldownTemp, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Cooldown");
    lv_obj_center(label);

    btn = lv_btn_create(one_above_bottom_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, btnToggleableEdit, LV_EVENT_CLICKED, NULL);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Edit Presets");
    lv_obj_center(label);

    lv_obj_scroll_to_y(rootTempPanel, 9999, LV_ANIM_OFF);
    lv_msg_send(DATA_PRINTER_TEMP_PRESET, &printer);
}