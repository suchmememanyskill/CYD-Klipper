#include "lvgl.h"
#include "panel.h"

#include "../../core/current_printer.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include "../ui_utils.h"
#include "../../core/lv_setup.h"
#include <UrlEncode.h>
#include "../../core/printer_integration.hpp"

const char* selected_file = NULL;

static void btn_print_file(lv_event_t * e)
{
    lv_obj_t * panel = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_del(panel);

    current_printer_start_file(selected_file);
}

static void btn_print_file_verify_instant(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    selected_file = (char*)lv_event_get_user_data(e);
    current_printer_start_file(selected_file);
}

static void btn_print_file_verify(lv_event_t * e)
{
    if (get_current_printer_data()->state != PrinterState::PrinterStateIdle){
        return;
    }

    selected_file = (char*)lv_event_get_user_data(e);
    
    lv_obj_t * panel = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_hor(panel, CYD_SCREEN_GAP_PX * 2, 0);
    lv_obj_set_style_pad_ver(panel, CYD_SCREEN_GAP_PX, 0);
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 4, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 3);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * label_print_file = lv_label_create(panel);
    lv_label_set_text(label_print_file, "Print File");

    lv_obj_t * top_panel = lv_create_empty_panel(panel);
    lv_obj_set_width(top_panel, LV_PCT(100));
    lv_obj_set_flex_grow(top_panel, 1);
    lv_layout_flex_row(top_panel, LV_FLEX_ALIGN_CENTER);

    Thumbnail thumbnail = current_printer_get_32_32_png_image_thumbnail(selected_file);
    lv_obj_t * img = NULL;

    if (thumbnail.success)
    {
        lv_img_dsc_t* img_header = (lv_img_dsc_t*)malloc(sizeof(lv_img_dsc_t));
        lv_obj_on_destroy_free_data(panel, img_header);

        memset(img_header, 0, sizeof(lv_img_dsc_t));
        img_header->header.w = 32;
        img_header->header.h = 32;
        img_header->data_size = thumbnail.size;
        img_header->header.cf = LV_IMG_CF_RAW_ALPHA;
        img_header->data = thumbnail.png;

        img = lv_img_create(top_panel);
        lv_img_set_src(img, img_header);
        lv_img_set_antialias(img, true);
        lv_img_set_size_mode(img, LV_IMG_SIZE_MODE_REAL);
        
        if (global_config.double_size_gcode_img)
        {
            lv_img_set_zoom(img, LV_IMG_ZOOM_NONE * 2);
        }
    }

    lv_obj_t * label = lv_label_create(top_panel);
    lv_label_set_text(label, selected_file);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_flex_grow(label, 1);

    lv_obj_t* buttons_panel = lv_create_empty_panel(panel);
    lv_layout_flex_row(buttons_panel);
    lv_obj_set_size(buttons_panel, LV_PCT(100), CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);

    lv_obj_t* btn = lv_btn_create(buttons_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Cancel");
    lv_obj_center(label);

    btn = lv_btn_create(buttons_panel);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, btn_print_file, LV_EVENT_CLICKED, panel);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_OK " Print");
    lv_obj_center(label);
}

void files_panel_init(lv_obj_t* panel){
    Files files = current_printer_get_files();

    if (!files.success || files.count <= 0){
        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, "Failed to read files.");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        return;
    }

    lv_obj_t * list = lv_list_create(panel);
    lv_obj_set_style_radius(list, 0, 0);
    lv_obj_set_style_border_width(list, 0, 0); 
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0); 
    lv_obj_set_size(list, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_PANEL_HEIGHT_PX);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

    for (int i = 0; i < files.count; i++)
    {
        lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_FILE, files.available_files[i]);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
              
        if (global_config.full_filenames)
        {
            lv_label_set_long_mode(lv_obj_get_child(btn, 1), LV_LABEL_LONG_WRAP);
        }

        lv_obj_add_event_cb(btn, (get_current_printer()->no_confirm_print_file) ? btn_print_file_verify_instant : btn_print_file_verify, LV_EVENT_CLICKED, (void*)(files.available_files[i]));
        lv_obj_on_destroy_free_data(btn, files.available_files[i]);
    }

    // Not deallocating filenames in this scope will cause double allocation, oh well.
    // TODO: read label text
    free(files.available_files);
}