#include "conf/global_config.h"
#include "core/screen_driver.h"
#include "ui/wifi_setup.h"
#include "ui/ip_setup.h"
#include "lvgl.h"
#include "core/data_setup.h"
#include "ui/main_ui.h"
#include "ui/nav_buttons.h"
#include <Esp.h>
#include "core/lv_setup.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Hello World");
    LoadGlobalConfig();
    screen_setup();
    lv_setup();
    Serial.println("Screen init done");
    
    wifi_init();
    ip_init();
    data_setup();

    nav_style_setup();
    main_ui_setup();
}

void loop(){
    wifi_ok();
    ip_ok();
    data_loop();
    lv_timer_handler();
    lv_task_handler();
}