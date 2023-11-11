#include "conf/global_config.h"
#include "conf/screen_driver.h"
#include "ui/wifi_setup.h"
#include "lvgl.h"

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

    lv_obj_clean(lv_scr_act());

    lv_obj_t * label;

    lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, 0);

    label = lv_label_create(btn1);
    lv_label_set_text(label, "Reset Configuration");
    lv_obj_center(label);
}

void loop(){
    lv_timer_handler();
    lv_task_handler();
}