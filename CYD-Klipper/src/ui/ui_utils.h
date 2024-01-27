#pragma once

#define CYD_SCREEN_PANEL_WIDTH \
    (CYD_SCREEN_WIDTH_PX - CYD_SCREEN_SIDEBAR_SIZE_PX)

lv_obj_t* lv_create_empty_panel(lv_obj_t* root);