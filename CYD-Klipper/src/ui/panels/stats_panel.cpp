#include "panel.h"
#include "../ui_utils.h"
#include "../../core/data_setup.h"
#include <stdio.h>
#include <Esp.h>

static void SetFanSpeedText(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Fan: %.0f%%", printer.fanSpeed * 100);
    lv_label_set_text(label, data);
}

static void SetFanSpeed(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[64];
    sprintf(gcode, "M106 S%d", speed);
    SendGcode(true, gcode);
}

const char* FAN_SPEEDS[] = { "0%", "15%", "25%", "35%" };
const int FAN_SPEEDS_VALUES[] = { 0, 38, 64, 90 };

const char* FAN_SPEEDS_2[] = { "50%", "75%", "100%"};
const int FAN_SPEEDS_VALUES_2[] = { 128, 192, 255 };

LvButtonColumn_t FAN_SPEED_COLUMNS[] = {
    { SetFanSpeed, FAN_SPEEDS, (const void**)FAN_SPEEDS_VALUES, 4},
    { SetFanSpeed, FAN_SPEEDS_2, (const void**)FAN_SPEEDS_VALUES_2, 3}
};

static void SetZOffsetText(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Z Offset: %.03f", printer.gcodeOffset[2]);
    lv_label_set_text(label, data);
}

static void SetZOffsetTextEx(lv_event_t * e) {
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Z Offset: %.03f, Z: %.03f", printer.gcodeOffset[2], printer.position[2]);
    lv_label_set_text(label, data);
}

static void SetZOffset(lv_event_t * e){
    char* offset = (char*)lv_event_get_user_data(e);

    char gcode[64];
    sprintf(gcode, "SET_GCODE_OFFSET Z_ADJUST=%s MOVE=1", offset);
    SendGcode(true, gcode);
}

static void setZ(lv_event_t * e){
    void* ptr = lv_event_get_user_data(e);
    float value = *(float *)(&ptr);

    if (value < 0) {
        SendGcode(true, "SET_GCODE_OFFSET Z=0 MOVE=1");
        return;
    }

    MovePrinter("Z", value, false);
}

const char* ZOFFSETS[] = { "-0.01", "-0.025", "-0.05", "-0.2" };
const char* ZOFFSETS_2[] = { "+0.01", "+0.025", "+0.05", "+0.2" };
const char* ZABS[] = { "Z=0", "Z=0.1", "Z=1", "Clear" };
const float ZABSVALUES[] = { 0, 0.1f, 1.0f, -1.0f };

LvButtonColumn_t ZOFFSET_COLUMNS[] = {
    { SetZOffset, ZOFFSETS, (const void**)ZOFFSETS, 4},
    { SetZOffset, ZOFFSETS_2, (const void**)ZOFFSETS_2, 4},
    { setZ, ZABS, (const void**)ZABSVALUES, 4}
};

static void SetSpeedMultText(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Speed: %.0f%%", printer.speedMult * 100);
    lv_label_set_text(label, data);
}

static void SetSpeedMult(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[64];
    sprintf(gcode, "M220 S%d", speed);
    SendGcode(true, gcode);
}

static void SetSpeedMultOffset(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    float result = printer.speedMult * 100 + speed;
    printer.speedMult = result / 100;
    char gcode[64];
    sprintf(gcode, "M220 S%.0f", result);
    SendGcode(true, gcode);
}

const char* SPEED_PRESETS[] = { "50%", "100%", "150%", "200%" };
const int SPEED_PRESETS_VALUES[] = { 50, 100, 150, 200 };
const char* SPEED_PRESETS_MINUS[] = { "-1%", "-5%", "-10%", "-25%" };
const int SPEED_PRESETS_MINUS_VALUES[] = { -1, -5, -10, -25 };
const char* SPEED_PRESETS_PLUS[] = { "+1%", "+5%", "+10%", "+25%" };
const int SPEED_PRESETS_PLUS_VALUES[] = { 1, 5, 10, 25 };

LvButtonColumn_t SPEED_MULT_COLUMNS[] = {
    { SetSpeedMult, SPEED_PRESETS, (const void**)SPEED_PRESETS_VALUES, 4},
    { SetSpeedMultOffset, SPEED_PRESETS_MINUS, (const void**)SPEED_PRESETS_MINUS_VALUES, 4},
    { SetSpeedMultOffset, SPEED_PRESETS_PLUS, (const void**)SPEED_PRESETS_PLUS_VALUES, 4}
};

static void SetExtrudeMultText(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char data[64];
    sprintf(data, "Flow: %.0f%%", printer.extrudeMult * 100);
    lv_label_set_text(label, data);
}

static void SetExtrudeMult(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    char gcode[64];
    sprintf(gcode, "M221 S%d", speed);
    SendGcode(true, gcode);
}

static void SetExtrudeMultOffset(lv_event_t * e){
    int speed = (int)lv_event_get_user_data(e);
    float result = printer.extrudeMult * 100 + speed;
    printer.extrudeMult = result / 100;
    char gcode[64];
    sprintf(gcode, "M221 S%.0f", result);
    SendGcode(true, gcode);
}

const char* EXTRUDE_PRESETS[] = { "95%", "100%", "105%", "110%" };
const int EXTRUDE_PRESETS_VALUES[] = { 95, 100, 105, 110 };
const char* EXTRUDE_OFFSET[] = { "+5%", "+1%", "-1%", "-5%" };
const int EXTRUDE_OFFSET_VALUES[] = { 5, 1, -1, -5 };

LvButtonColumn_t EXTRUDE_MULT_COLUMNS[] = {
    { SetExtrudeMult, EXTRUDE_PRESETS, (const void**)EXTRUDE_PRESETS_VALUES, 4},
    { SetExtrudeMultOffset, EXTRUDE_OFFSET, (const void**)EXTRUDE_OFFSET_VALUES, 4}
};

static void OpenFanSpeedPanel(lv_event_t * e){
    CreateFullscreenButtonMatrixPopup(lv_scr_act(), SetFanSpeedText, FAN_SPEED_COLUMNS, 2);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

static void OpenZOffsetPanel(lv_event_t * e){
    CreateFullscreenButtonMatrixPopup(lv_scr_act(), SetZOffsetTextEx, ZOFFSET_COLUMNS, (printer.state == PRINTER_STATE_IDLE) ? 3 : 2);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

static void OpenSpeedMultPanel(lv_event_t * e){
    CreateFullscreenButtonMatrixPopup(lv_scr_act(), SetSpeedMultText, SPEED_MULT_COLUMNS, 3);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

static void OpenExtrudeMultPanel(lv_event_t * e){
    CreateFullscreenButtonMatrixPopup(lv_scr_act(), SetExtrudeMultText, EXTRUDE_MULT_COLUMNS, 2);
    lv_msg_send(DATA_PRINTER_DATA, &printer);
}

void CreateStateButton(lv_obj_t * root, lv_event_cb_t label, lv_event_cb_t button){
    lv_obj_t * btn = lv_btn_create(root);
    lv_obj_set_size(btn, CYD_SCREEN_PANEL_WIDTH_PX / 2 - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, button, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_obj = lv_label_create(btn);
    lv_obj_add_event_cb(label_obj, label, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, label_obj, NULL);
    lv_obj_align(label_obj, LV_ALIGN_CENTER, 0, 0);
}

static void LabelPos(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char x_pos_buff[32];
    sprintf(x_pos_buff, "X%.2f Y%.2f", printer.position[0], printer.position[1]);
    lv_label_set_text(label, x_pos_buff);
}

static void LabelFilamentUsedM(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char filament_buff[32];
    sprintf(filament_buff, "%.2f m", printer.feedrateMmPerS / 1000);
    lv_label_set_text(label, filament_buff);
}

static void LlabelTotalLayers(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char layers_buff[32];
    sprintf(layers_buff, "%d of %d", printer.currentLayer, printer.totalLayers);
    lv_label_set_text(label, layers_buff);
}

static void LabelPressureAdvance(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char pressure_buff[32];
    sprintf(pressure_buff, "%.3f (%.2fs)", printer.pressureAdvance, printer.smoothTime);
    lv_label_set_text(label, pressure_buff);
}

static void LabelFeedrate(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char feedrate_buff[32];
    sprintf(feedrate_buff, "%d mm/s", printer.feedrateMmPerS);
    lv_label_set_text(label, feedrate_buff);
}

void CreateStatTextBlock(lv_obj_t * root, const char* label, lv_event_cb_t value){
    lv_obj_t * panel = CreateEmptyPanel(root);
    lv_obj_set_size(panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    LayoutFlexColumn(panel , LV_FLEX_ALIGN_START, CYD_SCREEN_GAP_PX / 2, CYD_SCREEN_GAP_PX / 2);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t * label_obj = lv_label_create(panel);
    lv_label_set_text(label_obj, label);
    lv_obj_set_style_text_font(label_obj, &CYD_SCREEN_FONT_SMALL, 0);

    lv_obj_t * value_obj = lv_label_create(panel);
    lv_obj_add_event_cb(value_obj, value, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subscribe_obj(DATA_PRINTER_DATA, value_obj, NULL);
}

void StatsPanelInit(lv_obj_t* panel) {
    auto panel_width = CYD_SCREEN_PANEL_WIDTH_PX / 2 - CYD_SCREEN_GAP_PX * 3;

    lv_obj_t * left_panel = CreateEmptyPanel(panel);
    lv_obj_set_size(left_panel, panel_width, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    LayoutFlexColumn(left_panel);
    lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_align(left_panel, LV_ALIGN_TOP_LEFT, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

    CreateStatTextBlock(left_panel, "Position:", LabelPos);

    if (printer.state != PRINTER_STATE_IDLE){
        CreateStatTextBlock(left_panel, "Filament Used:", LabelFilamentUsedM);
        CreateStatTextBlock(left_panel, "Layer:", LlabelTotalLayers);
    }

    CreateStatTextBlock(left_panel, "Pressure Advance:", LabelPressureAdvance);

    CreateStatTextBlock(left_panel, "Feedrate:", LabelFeedrate);

    lv_obj_t * right_panel = CreateEmptyPanel(panel);
    lv_obj_set_size(right_panel, panel_width, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 2);
    LayoutFlexColumn(right_panel, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(right_panel, LV_ALIGN_TOP_RIGHT, -1 * CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);
    
    CreateStateButton(right_panel, SetFanSpeedText, OpenFanSpeedPanel);
    CreateStateButton(right_panel, SetZOffsetText, OpenZOffsetPanel);
    CreateStateButton(right_panel, SetSpeedMultText, OpenSpeedMultPanel);
    CreateStateButton(right_panel, SetExtrudeMultText, OpenExtrudeMultPanel);
}