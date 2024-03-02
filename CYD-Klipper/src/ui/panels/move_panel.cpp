#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"
#include "../nav_buttons.h"
#include "../ui_utils.h"
#include <stdio.h>

static bool lastHomingState = false;

static void XLineButtonPress(lv_event_t * e) {
    float* dataPointer = (float*)lv_event_get_user_data(e);
    float data = *dataPointer;
    movePrinter("X", data, true);
}

static void YLineButtonPress(lv_event_t * e) {
    float* dataPointer = (float*)lv_event_get_user_data(e);
    float data = *dataPointer;
    movePrinter("Y", data, true);
}

static void ZLineButtonPress(lv_event_t * e) {
    float* dataPointer = (float*)lv_event_get_user_data(e);
    float data = *dataPointer;
    movePrinter("Z", data, true);
}

char xPosBuff[12];

static void XPosUpdate(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(xPosBuff, "X: %.1f", printer.position[0]);
    lv_label_set_text(label, xPosBuff);
}

char yPosBuff[12];

static void YPosUpdate(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(yPosBuff, "Y: %.1f", printer.position[1]);
    lv_label_set_text(label, yPosBuff);
}

char zPosBuff[12];

static void ZPosUpdate(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    sprintf(zPosBuff, "Z: %.2f", printer.position[2]);
    lv_label_set_text(label, zPosBuff);
}

lv_event_cb_t buttonCallbacks[] = {XLineButtonPress, YLineButtonPress, ZLineButtonPress};
lv_event_cb_t positionCallbacks[] = {XPosUpdate, YPosUpdate, ZPosUpdate};

const float XY_OFFSETS[] = {-100, -10, -1, 1, 10, 100};
const float Z_OFFSETS[] = {-10, -1, -0.1, 0.1, 1, 10};
const float* OFFSETS[] = {
    XY_OFFSETS,
    XY_OFFSETS,
    Z_OFFSETS
};

const char* XY_OFFSET_LABELS[] = {"-100", "-10", "-1", "+1", "+10", "+100"};
const char* Z_OFFSET_LABELS[] = {"-10", "-1", "-0.1", "+0.1", "+1", "+10"};

const char** OFFSET_LABELS[] = {
    XY_OFFSET_LABELS,
    XY_OFFSET_LABELS,
    Z_OFFSET_LABELS
};

static void HomeButtonClick(lv_event_t * e) {
    if (printer.state == PRINTER_STATE_PRINTING)
        return;

    sendGcode(false, "G28");
}

static void DisableSteppersClick(lv_event_t * e) {
    if (printer.state == PRINTER_STATE_PRINTING)
        return;

    sendGcode(true, "M18");
}

static void SwitchToStatPanel(lv_event_t * e) {
    lv_obj_t * panel = lv_event_get_target(e);
    nav_buttons_setup(5);
}

inline void RootPanelSteppersLocked(lv_obj_t * rootPanel){
    const auto width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * panel = CreateEmptyPanel(rootPanel);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    LayoutFlexColumn(panel, LV_FLEX_ALIGN_SPACE_BETWEEN, 0, 0);

    lv_obj_t * homeButtonRow = CreateEmptyPanel(panel);
    lv_obj_set_size(homeButtonRow, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    LayoutFlexRow(homeButtonRow);

    lv_obj_t * btn = lv_btn_create(homeButtonRow);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, HomeButtonClick, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME "Home");
    lv_obj_center(label);

    btn = lv_btn_create(homeButtonRow);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, DisableSteppersClick, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_EYE_CLOSE " Unlock");
    lv_obj_center(label);

    btn = lvBtnCreate(homeButtonRow);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, SwitchToStatPanel, LV_EVENT_CLICKED, NULL);
    lv_obj_set_flex_grow(btn, 1);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_EDIT " Params");
    lv_obj_center(label);

    for (int row = 0; row < 3; row++) {
        label = lv_label_create(panel);
        lv_label_set_text(label, "???");
        lv_obj_set_width(label, width);
        lv_obj_add_event_cb(label, positionCallbacks[row], LV_EVENT_MSG_RECEIVED, NULL);
        lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

        lv_obj_t * rowPanel = CreateEmptyPanel(panel);
        lv_obj_set_size(rowPanel, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        LayoutFlexRow(rowPanel);

        for (int col = 0; col < 6; col++)
        {
            btn = lvBtnCreate(rowPanel);
            lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
            lv_obj_add_event_cb(btn, buttonCallbacks[row], LV_EVENT_CLICKED, (void*)(OFFSETS[row] + col));
            lv_obj_set_flex_grow(btn, 1);

            label = lv_label_create(btn);
            lv_label_set_text(label, OFFSET_LABELS[row][col]);
            lv_obj_center(label);
        }
    }

}

inline void RootPanelSteppersUnlocked(lv_obj_t * rootPanel){
    lv_obj_t * panel = CreateEmptyPanel(rootPanel);
    lv_obj_set_size(panel, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    LayoutFlexColumn(panel, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, LV_SYMBOL_EYE_CLOSE " Steppers unlocked");

    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, HomeButtonClick, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME "Home Axis");
    lv_obj_center(label);
}

static void RootPanelStateUpdate(lv_event_t * e){
    if (lastHomingState == printer.homedAxis)
        return;

    lv_obj_t * panel = lv_event_get_target(e);
    lastHomingState = printer.homedAxis;

    lv_obj_clean(panel);

    if (printer.homedAxis)
        RootPanelSteppersLocked(panel);
    else
        RootPanelSteppersUnlocked(panel);
}

void MovePanelInit(lv_obj_t* panel){
    if (printer.state == PRINTER_STATE_PRINTING){
        stats_panel_init(panel);
        return;
    }

    lastHomingState = !printer.homedAxis;

    lv_obj_add_event_cb(panel, RootPanelStateUpdate, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, panel, NULL);
}
