#include "panel.h"
#include "../../core/data_setup.h"
#include <stdio.h>
#include "../ui_utils.h"

char timeBuffer[12];

char* timeDisplay(unsigned long time){
    unsigned long hours = time / 3600;
    unsigned long minutes = (time % 3600) / 60;
    unsigned long seconds = (time % 3600) % 60;
    sprintf(timeBuffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return timeBuffer;
}

static void progressBarUpdate(lv_event_t* e){
    lv_obj_t * bar = lv_event_get_target(e);
    lv_bar_set_value(bar, printer.printProgress * 100, LV_ANIM_ON);
}

static void updatePrinterDataElapsedTime(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, timeDisplay(printer.elapsedTime));
}

static void updatePrinterDataRemainingTime(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, timeDisplay(printer.remainingTime));
}

static void updatePrinterDataPercentage(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char percentageBuffer[12];
    sprintf(percentageBuffer, "%.2f%%", printer.printProgress * 100);
    lv_label_set_text(label, percentageBuffer);
}

static void btnClickStop(lv_event_t * e){
    sendGcode(true, "CANCEL_PRINT");
}

static void btnClickPause(lv_event_t * e){
    sendGcode(true, "PAUSE");
}

static void btnClickResume(lv_event_t * e){
    sendGcode(true, "RESUME");
}

void progressPanelInit(lv_obj_t* panel){
    auto panelWidth = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3;
    const auto buttonSizeMult = 1.3f;

    lv_obj_t * centerPanel = CreateEmptyPanel(panel);
    lv_obj_align(centerPanel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(centerPanel, panelWidth, LV_SIZE_CONTENT);
    LayoutFlexColumn(centerPanel);

    // Filename
    lv_obj_t * label = lv_label_create(centerPanel);
    lv_label_set_text(label, printer.printFilename);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, panelWidth);

    // Progress Bar
    lv_obj_t * bar = lv_bar_create(centerPanel);
    lv_obj_set_size(bar, panelWidth, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * 0.75f);
    lv_obj_add_event_cb(bar, progressBarUpdate, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, bar, NULL);

    // Time
    lv_obj_t * timeEstPanel = CreateEmptyPanel(centerPanel);
    lv_obj_set_size(timeEstPanel, panelWidth, LV_SIZE_CONTENT);

    // Elapsed Time
    label = lv_label_create(timeEstPanel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(label, updatePrinterDataElapsedTime, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Remaining Time
    label = lv_label_create(timeEstPanel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(label, updatePrinterDataRemainingTime, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Percentage
    label = lv_label_create(timeEstPanel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, updatePrinterDataPercentage, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Stop Button
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -1 * CYD_SCREEN_GAP_PX, -1 * CYD_SCREEN_GAP_PX);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * buttonSizeMult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * buttonSizeMult);
    lv_obj_add_event_cb(btn, btnClickStop, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_STOP);
    lv_obj_center(label);

    // Resume Button
    if (printer.state == PRINTER_STATE_PAUSED){
        btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btnClickResume, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PLAY);
        lv_obj_center(label);
    }
    // Pause Button
    else {
        btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btnClickPause, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PAUSE);
        lv_obj_center(label);
    }

    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -2 * CYD_SCREEN_GAP_PX - CYD_SCREEN_MIN_BUTTON_WIDTH_PX * buttonSizeMult, -1 * CYD_SCREEN_GAP_PX);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * buttonSizeMult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * buttonSizeMult);
}
