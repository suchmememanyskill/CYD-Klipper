#pragma once

#include "lvgl.h"
#include "../../core/macros_query.h"

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

void SettingsPanelInit(lv_obj_t* panel);
void TempPanelInit(lv_obj_t* panel);
void PrintPanelInit(lv_obj_t* panel);
void MovePanelInit(lv_obj_t* panel);
void ProgressPanelInit(lv_obj_t* panel);
void MacrosPanelInit(lv_obj_t* panel);
void StatsPanelInit(lv_obj_t* panel);
void MacrosPanelAddPowerDevicesToPanel(lv_obj_t * panel, PowerQueryT query);
