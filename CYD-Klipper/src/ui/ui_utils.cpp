#include "lvgl.h"
#include "ui_utils.h"

lv_obj_t* lv_create_empty_panel(lv_obj_t* root) {
    lv_obj_t* panel = lv_obj_create(root); 
    lv_obj_set_style_border_width(panel, 0, 0); 
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_pad_all(panel, 0, 0);
    return panel;
}

void lv_layout_flex_column(lv_obj_t* obj, lv_flex_align_t allign, lv_coord_t pad_column, lv_coord_t pad_row){
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, allign, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(obj, pad_column, 0);
    lv_obj_set_style_pad_row(obj, pad_row, 0);
}

void lv_layout_flex_row(lv_obj_t* obj, lv_flex_align_t allign, lv_coord_t pad_column, lv_coord_t pad_row){
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, allign, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(obj, pad_column, 0);
    lv_obj_set_style_pad_row(obj, pad_row, 0);
}