#include "lvgl.h"
#include "ui_utils.h"
#include "../core/data_setup.h"

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

static void lv_fullscreen_menu_close(lv_event_t * e){
    lv_obj_t * panel = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_del(panel);
}

void lv_create_fullscreen_button_matrix_popup(lv_obj_t * root, lv_event_cb_t title, lv_button_column_t* columns, int column_count){
    const auto full_panel_width = CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 3;
    const auto full_panel_inner_width = full_panel_width - CYD_SCREEN_GAP_PX * 2 - 4;
    const auto full_panel_height = CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX;
    const auto full_panel_inner_height = full_panel_height - CYD_SCREEN_GAP_PX * 2 - 4;
    auto column_width = full_panel_inner_width / column_count - CYD_SCREEN_GAP_PX;
    auto column_height = full_panel_inner_height - CYD_SCREEN_GAP_PX - CYD_SCREEN_MIN_BUTTON_HEIGHT_PX;

    lv_obj_t * panel = lv_obj_create(root);
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX, 0);
    lv_obj_set_size(panel, full_panel_width, full_panel_height);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * top_menu_row = lv_create_empty_panel(panel);
    lv_obj_set_size(top_menu_row, full_panel_inner_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(top_menu_row, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t * btn = lv_btn_create(top_menu_row);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btn, lv_fullscreen_menu_close, LV_EVENT_CLICKED, panel);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_center(label);

    label = lv_label_create(top_menu_row);
    lv_label_set_text(label, "-");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(label, title, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    
    lv_obj_t * rows = lv_create_empty_panel(panel);
    lv_obj_set_size(rows, full_panel_inner_width, column_height);
    lv_obj_align(rows, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_layout_flex_row(rows, LV_FLEX_ALIGN_SPACE_BETWEEN, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

    for (int i = 0; i < column_count; i++){
        lv_obj_t * column = lv_create_empty_panel(rows);
        lv_obj_clear_flag(column, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(column, column_width, column_height);
        lv_layout_flex_column(column, LV_FLEX_ALIGN_CENTER, CYD_SCREEN_GAP_PX, CYD_SCREEN_GAP_PX);

        for (int j = 0; j < columns[i].length; j++){
            lv_obj_t * btn = lv_btn_create(column);
            lv_obj_set_size(btn, column_width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
            lv_obj_add_event_cb(btn, columns[i].event, LV_EVENT_CLICKED, (void*)columns[i].data[j]);

            label = lv_label_create(btn);
            lv_label_set_text(label, columns[i].labels[j]);
            lv_obj_center(label);
        }
    }
}