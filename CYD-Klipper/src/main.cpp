#include "conf/global_config.h"
#include "core/screen_driver.h"
#include "ui/wifi_setup.h"
#include "ui/ip_setup.h"
#include "ui/serial/serial_console.h"
#include "lvgl.h"
#include "core/data_setup.h"
#include "ui/main_ui.h"
#include "ui/nav_buttons.h"
#include <Esp.h>
#include "core/lv_setup.h"
#include "ui/ota_setup.h"

void setup() {
    Serial.begin(115200);
    serial_console::greet();
    load_global_config();
    screen_setup();
    lv_setup();
    LOG_LN("Screen init done");
    
    wifi_init();
    ota_init();
    ip_init();
    data_setup();

    nav_style_setup();
    main_ui_setup();
}

void loop(){
    wifi_ok();
    data_loop();
    lv_handler();
    serial_console::run();

    if (is_ready_for_ota_update())
    {
        ota_do_update();
    }
}