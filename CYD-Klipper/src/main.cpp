#include "conf/global_config.h"
#include "core/screen_driver.h"
#include "ui/wifi_setup.h"
#include "ui/ip_setup.h"
#include "lvgl.h"
#include "core/data_setup.h"
#include "ui/main_ui.h"
#include "ui/nav_buttons.h"

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
    data_setup();

    nav_style_setup();
    main_ui_setup();
}

void loop(){
    wifi_ok();
    data_loop();
    lv_timer_handler();
    lv_task_handler();
}