#pragma once

#ifdef CYD_SCREEN_VERTICAL
    #ifndef CYD_SCREEN_WIDTH_PX
        #define CYD_SCREEN_WIDTH_PX LCD_WIDTH
    #endif

    #ifndef CYD_SCREEN_HEIGHT_PX
        #define CYD_SCREEN_HEIGHT_PX LCD_HEIGHT
    #endif

    #define CYD_SCREEN_PANEL_HEIGHT_PX \
        (CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_SIDEBAR_SIZE_PX)
    #define CYD_SCREEN_PANEL_WIDTH_PX CYD_SCREEN_WIDTH_PX
#else 
    #ifndef CYD_SCREEN_WIDTH_PX
        #define CYD_SCREEN_WIDTH_PX LCD_HEIGHT
    #endif

    #ifndef CYD_SCREEN_HEIGHT_PX
        #define CYD_SCREEN_HEIGHT_PX LCD_WIDTH
    #endif

    #define CYD_SCREEN_PANEL_HEIGHT_PX CYD_SCREEN_HEIGHT_PX
    #define CYD_SCREEN_PANEL_WIDTH_PX \
        (CYD_SCREEN_WIDTH_PX - CYD_SCREEN_SIDEBAR_SIZE_PX)
#endif

typedef struct {
    lv_event_cb_t event;
    const char** labels;
    const void** data;
    int length;
} lv_button_column_t;

lv_obj_t* lv_create_empty_panel(lv_obj_t* root);
void lv_layout_flex_column(lv_obj_t* obj, lv_flex_align_t allign = LV_FLEX_ALIGN_START, lv_coord_t pad_column = CYD_SCREEN_GAP_PX, lv_coord_t pad_row = CYD_SCREEN_GAP_PX);
void lv_layout_flex_row(lv_obj_t* obj, lv_flex_align_t allign = LV_FLEX_ALIGN_START, lv_coord_t pad_column = CYD_SCREEN_GAP_PX, lv_coord_t pad_row = CYD_SCREEN_GAP_PX);
void lv_create_fullscreen_button_matrix_popup(lv_obj_t * root, lv_event_cb_t title, lv_button_column_t* columns, int column_count);
void destroy_event_user_data(lv_event_t * e);