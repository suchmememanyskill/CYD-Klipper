#include "lvgl.h"
#include "../../core/macros_query.h"

#define SIZEOF(arr) (sizeof(arr) / sizeof(*arr))

void settingsPanelInit(lv_obj_t* panel);
void tempPanelInit(lv_obj_t* panel);
void printPanelInit(lv_obj_t* panel);
void movePanelInit(lv_obj_t* panel);
void progressPanelInit(lv_obj_t* panel);
void macrosPanelInit(lv_obj_t* panel);
void statsPanelInit(lv_obj_t* panel);
void macrosPanelAddPowerDevicesToPanel(lv_obj_t * panel, PowerQuery query);
