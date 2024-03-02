#pragma once

#ifndef CYD_SCREEN_WIDTH_PX
#define CYD_SCREEN_WIDTH_PX LCD_HEIGHT
#endif

#ifndef CYD_SCREEN_HEIGHT_PX
#define CYD_SCREEN_HEIGHT_PX LCD_WIDTH
#endif

#define CYD_SCREEN_PANEL_WIDTH_PX \
    (CYD_SCREEN_WIDTH_PX - CYD_SCREEN_SIDEBAR_SIZE_PX)

typedef struct {
    lv_event_cb_t event;
    const char** labels;
    const void** data;
    int length;
} LvButtonColumn_t;

lv_obj_t* CreateEmptyPanel(lv_obj_t* root);
void LayoutFlexColumn(lv_obj_t* obj, lv_flex_align_t align = LV_FLEX_ALIGN_START, lv_coord_t pad_column = CYD_SCREEN_GAP_PX, lv_coord_t pad_row = CYD_SCREEN_GAP_PX);
void LayoutFlexRow(lv_obj_t* obj, lv_flex_align_t align = LV_FLEX_ALIGN_START, lv_coord_t pad_column = CYD_SCREEN_GAP_PX, lv_coord_t pad_row = CYD_SCREEN_GAP_PX);
void CreateFullscreenButtonMatrixPopup(lv_obj_t * root, lv_event_cb_t title, LvButtonColumn_t* columns, int column_count);
void DestroyEventUserData(lv_event_t * e);
