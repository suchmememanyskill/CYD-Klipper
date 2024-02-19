#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"
#include "../../core/files_query.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include <HTTPClient.h>
#include "../ui_utils.h"
#include "../../core/lv_setup.h"

FILESYSTEM_FILE* selected_file = NULL;

static void btn_print_file(lv_event_t * e){
    lv_obj_t * panel = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_del(panel);

    char* buff = (char*)malloc(128 + (strlen(selected_file->name) * 3));
    sprintf(buff, "http://%s:%d/printer/print/start?filename=", global_config.klipperHost, global_config.klipperPort);

    char* ptr = buff + strlen(buff);
    int filename_length = strlen(selected_file->name);
    for (int i = 0; i < filename_length; i++){
        char c = selected_file->name[i];
        if (c == ' '){
            *ptr = '%';
            ptr++;
            *ptr = '2';
            ptr++;
            *ptr = '0';
        } else {
            *ptr = c;
        }
        ptr++;
    }

    *ptr = 0;

    HTTPClient client;
    client.begin(buff);

    if (global_config.auth_configured)
        client.addHeader("X-Api-Key", global_config.klipper_auth);

    int httpCode = client.POST("");
    Serial.printf("Print start: HTTP %d\n", httpCode);
}

static void btn_print_file_verify(lv_event_t * e){
    const auto button_size_mult = 1.3f;

    lv_obj_t * btn = lv_event_get_target(e);
    selected_file = (FILESYSTEM_FILE*)lv_event_get_user_data(e);
    
    lv_obj_t * panel = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_all(panel, CYD_SCREEN_GAP_PX * 2, 0);
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 4, CYD_SCREEN_HEIGHT_PX - CYD_SCREEN_GAP_PX * 3);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, "Print File");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

    label = lv_label_create(panel);
    lv_label_set_text(label, selected_file->name);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_width(label, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 10);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * button_size_mult);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_center(label);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * button_size_mult);
    lv_obj_add_event_cb(btn, btn_print_file, LV_EVENT_CLICKED, panel);
    
    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_OK);
    lv_obj_center(label);
}

void print_panel_init(lv_obj_t* panel){
    if (printer.state == PRINTER_STATE_PRINTING || printer.state == PRINTER_STATE_PAUSED){
        progress_panel_init(panel);
        return;
    }

    lv_obj_t * list = lv_list_create(panel);
    lv_obj_set_style_radius(list, 0, 0);
    lv_obj_set_style_border_width(list, 0, 0); 
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0); 
    lv_obj_set_size(list, CYD_SCREEN_PANEL_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

    FILESYSTEM_FILE* files = get_files(25);
    int count = 0;
    while (files != NULL && files->name != NULL && count <= 20){
        lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_FILE, files->name);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0); 
        lv_obj_add_event_cb(btn, btn_print_file_verify, LV_EVENT_CLICKED, (void*)files);

        files += 1;
        count++;
    }

    if (count <= 0){
        lv_obj_del(list);
        lv_obj_t * label = lv_label_create(panel);
        lv_label_set_text(label, "Failed to read files.");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
}