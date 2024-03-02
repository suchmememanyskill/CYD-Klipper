#include "lvgl.h"
#include "panel.h"
#include "../../core/screen_driver.h"
#include "../../conf/global_config.h"
#include "../main_ui.h"
#include "../ui_utils.h"
#include <Esp.h>
#include "../../core/lv_setup.h"
#include "../ota_setup.h"

#ifndef REPO_VERSION
#define REPO_VERSION "Unknown"
#endif // REPO_VERSION

static void InvertColorSwitch(lv_event_t *e)
{
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    globalConfig.invertColors = checked;
    WriteGlobalConfig();
    SetInvertDisplay();
}

static void ResetCalibrationClick(lv_event_t *e)
{
    globalConfig.screenCalibrated = false;
    WriteGlobalConfig();
    ESP.restart();
}

static void ResetWifiClick(lv_event_t *e)
{
    globalConfig.wifiConfigured = false;
    globalConfig.ipConfigured = false;
    globalConfig.authConfigured = false;
    WriteGlobalConfig();
    ESP.restart();
}

static void LightModeSwitch(lv_event_t *e)
{
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    globalConfig.lightMode = checked;
    WriteGlobalConfig();
    SetColorScheme();
}

static void ThemeDropdown(lv_event_t *e)
{
    lv_obj_t *dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    globalConfig.colorScheme = selected;
    SetColorScheme();
    WriteGlobalConfig();
}

const char *brightness_options = "100%\n75%\n50%\n25%";
const char brightness_options_values[] = {255, 192, 128, 64};

static void BrightnessDropdown(lv_event_t *e)
{
    lv_obj_t *dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    globalConfig.brightness = brightness_options_values[selected];
    SetScreenBrightness();
    WriteGlobalConfig();
}

const char *wake_timeout_options = "1m\n2m\n5m\n10m\n15m\n30m\n1h\n2h\n4h";
const char wake_timeout_options_values[] = {1, 2, 5, 10, 15, 30, 60, 120, 240};

static void WakeTimeoutDropdown(lv_event_t *e)
{
    lv_obj_t *dropdown = lv_event_get_target(e);
    auto selected = lv_dropdown_get_selected(dropdown);
    globalConfig.screenTimeout = wake_timeout_options_values[selected];
    SetScreenTimerPeriod();
    WriteGlobalConfig();
}

static void RotateScreenSwitch(lv_event_t *e)
{
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    globalConfig.rotateScreen = checked;
    globalConfig.screenCalibrated = false;
    WriteGlobalConfig();
    ESP.restart();
}

static void OnDuringPrintSwitch(lv_event_t *e)
{
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    globalConfig.onDuringPrint = checked;
    CheckIfScreenNeedsToBeDisabled();
    WriteGlobalConfig();
}

static void BtnOtaDoUpdate(lv_event_t *e)
{
    SetReadyForOtaUpdate();
}

static void AutoOtaUpdateSwitch(lv_event_t *e)
{
    auto state = lv_obj_get_state(lv_event_get_target(e));
    bool checked = (state & LV_STATE_CHECKED == LV_STATE_CHECKED);
    globalConfig.autoOtaUpdate = checked;
    WriteGlobalConfig();
}

const char *EstimatedTimeDropdownOptions = "Percentage\nInterpolated\nSlicer";

static void
EstimatedTimeDropdown(lv_event_t * e)
{
lv_obj_t *dropdown = lv_event_get_target(e);
globalConfig.remainingTimeCalcMode = lv_dropdown_get_selected(dropdown);
WriteGlobalConfig();
}

const static lv_point_t line_points[] = {{0, 0}, {(short int)((CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2) * 0.85f), 0}};

void CreateSettingsWidget(const char *label_text, lv_obj_t *object, lv_obj_t *root_panel, bool set_height = true)
{
    lv_obj_t *panel = CreateEmptyPanel(root_panel);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_set_parent(object, panel);
    lv_obj_align(object, LV_ALIGN_RIGHT_MID, 0, 0);

    if (set_height)
        lv_obj_set_height(object, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t *line = lv_line_create(root_panel);
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0xAAAAAA), 0);
}

void SettingsPanelInit(lv_obj_t *panel)
{
    lv_obj_t *toggle = NULL;
    lv_obj_t *btn = NULL;
    lv_obj_t *label = NULL;
    lv_obj_t *dropdown = NULL;

    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    LayoutFlexColumn(panel);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);

    btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, ResetWifiClick, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    CreateSettingsWidget("Configure WiFi", btn, panel);

#ifndef CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
    btn = lv_btn_create(panel);
    lv_obj_add_event_cb(btn, ResetCalibrationClick, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, "Restart");
    lv_obj_center(label);

    CreateSettingsWidget("Calibrate Touch", btn, panel);
#endif // CYD_SCREEN_DISABLE_TOUCH_CALIBRATION

#ifndef CYD_SCREEN_DISABLE_INVERT_COLORS
    toggle = lv_switch_create(panel);
    lv_obj_set_width(toggle, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2);
    lv_obj_add_event_cb(toggle, InvertColorSwitch, LV_EVENT_VALUE_CHANGED, NULL);

    if (globalConfig.invertColors)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    CreateSettingsWidget("Invert Colors", toggle, panel);
#endif // CYD_SCREEN_DISABLE_INVERT_COLORS

    toggle = lv_switch_create(panel);
    lv_obj_set_width(toggle, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2);
    lv_obj_add_event_cb(toggle, LightModeSwitch, LV_EVENT_VALUE_CHANGED, NULL);

    if (globalConfig.lightMode)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    CreateSettingsWidget("Light Mode", toggle, panel);

    dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, "Blue\nGreen\nGrey\nYellow\nOrange\nRed\nPurple");
    lv_dropdown_set_selected(dropdown, globalConfig.colorScheme);
    lv_obj_add_event_cb(dropdown, ThemeDropdown, LV_EVENT_VALUE_CHANGED, NULL);

    CreateSettingsWidget("Theme", dropdown, panel);

    dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, brightness_options);
    lv_obj_add_event_cb(dropdown, BrightnessDropdown, LV_EVENT_VALUE_CHANGED, NULL);

    for (int i = 0; i < SIZEOF(brightness_options_values); i++)
    {
        if (brightness_options_values[i] == globalConfig.brightness)
        {
            lv_dropdown_set_selected(dropdown, i);
            break;
        }
    }

    CreateSettingsWidget("Brightness", dropdown, panel);

#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, wake_timeout_options);
    lv_obj_add_event_cb(dropdown, WakeTimeoutDropdown, LV_EVENT_VALUE_CHANGED, NULL);

    for (int i = 0; i < SIZEOF(wake_timeout_options_values); i++)
    {
        if (wake_timeout_options_values[i] == globalConfig.screenTimeout)
        {
            lv_dropdown_set_selected(dropdown, i);
            break;
        }
    }

    CreateSettingsWidget("Wake Timeout", dropdown, panel);
#endif

    dropdown = lv_dropdown_create(panel);
    lv_dropdown_set_options(dropdown, EstimatedTimeDropdownOptions);
    lv_obj_add_event_cb(dropdown, EstimatedTimeDropdown, LV_EVENT_VALUE_CHANGED, NULL);

    lv_dropdown_set_selected(dropdown, globalConfig.remainingTimeCalcMode);
    CreateSettingsWidget("Estimated Time", dropdown, panel);

    toggle = lv_switch_create(panel);
    lv_obj_set_width(toggle, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2);
    lv_obj_add_event_cb(toggle, RotateScreenSwitch, LV_EVENT_VALUE_CHANGED, NULL);

    if (globalConfig.rotateScreen)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    CreateSettingsWidget("Rotate Screen", toggle, panel);

#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    toggle = lv_switch_create(panel);
    lv_obj_set_width(toggle, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2);
    lv_obj_add_event_cb(toggle, OnDuringPrintSwitch, LV_EVENT_VALUE_CHANGED, NULL);

    if (globalConfig.onDuringPrint)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    CreateSettingsWidget("Screen On During Print", toggle, panel);
#endif

    toggle = lv_switch_create(panel);
    lv_obj_set_width(toggle, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * 2);
    lv_obj_add_event_cb(toggle, AutoOtaUpdateSwitch, LV_EVENT_VALUE_CHANGED, NULL);

    if (globalConfig.autoOtaUpdate)
        lv_obj_add_state(toggle, LV_STATE_CHECKED);

    CreateSettingsWidget("Auto Update", toggle, panel);

    label = lv_label_create(panel);
    lv_label_set_text(label, REPO_VERSION " ");

    CreateSettingsWidget("Version", label, panel, false);

    if (OtaHasUpdate())
    {
        btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, BtnOtaDoUpdate, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "Update to %s", OtaNewVersionName().c_str());
        lv_obj_center(label);

        CreateSettingsWidget("Device", btn, panel);
    }
    else
    {
        label = lv_label_create(panel);
        lv_label_set_text(label, ARDUINO_BOARD "  ");

        CreateSettingsWidget("Device", label, panel, false);
    }
}