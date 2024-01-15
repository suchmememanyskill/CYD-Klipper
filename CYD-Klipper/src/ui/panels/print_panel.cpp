#include "lvgl.h"
#include "panel.h"
#include "../../core/data_setup.h"
#include "../../core/files_query.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include <HTTPClient.h>

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
    int httpCode = client.POST("");
    Serial.printf("Print start: HTTP %d\n", httpCode);
}

static void btn_print_back(lv_event_t * e){
    lv_obj_t * panel = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_del(panel);
}

static void btn_print_file_verify(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    selected_file = (FILESYSTEM_FILE*)lv_event_get_user_data(e);
    
    lv_obj_t * panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(panel, TFT_HEIGHT - 40, TFT_WIDTH - 30);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * label = lv_label_create(panel);
    lv_label_set_text(label, "Print File");
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

    label = lv_label_create(panel);
    lv_label_set_text(label, selected_file->name);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_width(label, TFT_HEIGHT - 90);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(btn, 40, 40);
    lv_obj_add_event_cb(btn, btn_print_back, LV_EVENT_CLICKED, panel);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_center(label);

    btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_size(btn, 40, 40);
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

    auto panel_width = TFT_HEIGHT - 40;
    auto panel_height_margin = TFT_WIDTH - 10;
    auto panel_width_margin = panel_width - 10;

    lv_obj_t * list = lv_list_create(panel);
    lv_obj_set_size(list, panel_width_margin, panel_height_margin);
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

    FILESYSTEM_FILE* files = get_files(25);
    int count = 0;
    while (files != NULL && files->name != NULL && count <= 20){
        lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_FILE, files->name);
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