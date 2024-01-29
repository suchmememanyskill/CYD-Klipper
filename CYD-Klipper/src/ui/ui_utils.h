#pragma once

#define CYD_SCREEN_PANEL_WIDTH_PX \
    (CYD_SCREEN_WIDTH_PX - CYD_SCREEN_SIDEBAR_SIZE_PX)

lv_obj_t* lv_create_empty_panel(lv_obj_t* root);
void lv_layout_flex_column(lv_obj_t* obj, lv_flex_align_t allign = LV_FLEX_ALIGN_START, lv_coord_t pad_column = CYD_SCREEN_GAP_PX, lv_coord_t pad_row = CYD_SCREEN_GAP_PX);
void lv_layout_flex_row(lv_obj_t* obj, lv_flex_align_t allign = LV_FLEX_ALIGN_START, lv_coord_t pad_column = CYD_SCREEN_GAP_PX, lv_coord_t pad_row = CYD_SCREEN_GAP_PX);