#include "lvgl.h"
#include "ui_utils.h"

lv_obj_t* lv_create_empty_panel(lv_obj_t* root) {
    lv_obj_t* panel = lv_obj_create(root); 
    lv_obj_set_style_border_width(panel, 0, 0); 
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_pad_all(panel, 0, 0);
    return panel;
}