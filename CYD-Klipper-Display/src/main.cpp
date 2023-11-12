#include "conf/global_config.h"
#include "conf/screen_driver.h"
#include "ui/wifi_setup.h"
#include "ui/ip_setup.h"
#include "core/websocket_setup.h"
#include "lvgl.h"
#include "core/data_setup.h"
#include "ui/main_ui.h"

static void event_handler(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        global_config.version = 0;
        WriteGlobalConfig();
        ESP.restart();
    }
}


void setup() {
    Serial.begin(115200);
    Serial.println("Hello World");
    LoadGlobalConfig();
    screen_setup();
    Serial.println("Screen init done");
    
    wifi_init();
    ip_setup();
    websocket_setup();
    data_setup();
    main_ui_setup();

    
    
    /*
    lv_obj_clean(lv_scr_act());
    
    lv_obj_t * label;

    lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, 0);

    label = lv_label_create(btn1);
    lv_label_set_text(label, "Reset Configuration");
    lv_obj_center(label);

    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 40);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    */
}

void loop(){
    wifi_ok();
    data_loop();
    lv_timer_handler();
    lv_task_handler();
}