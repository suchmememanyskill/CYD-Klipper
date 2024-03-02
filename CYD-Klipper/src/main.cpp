
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
#include "ui/ota_setup.h"

void setup() {
    Serial.begin(115200);
    Serial.println("Hello World");
    LoadGlobalConfig();
    SetupScreen();
    LvSetup();
    Serial.println("Screen init done");
    
    WifiInit();
    OtaInit();
    IpInit();
    DataSetup();

    NavStyleSetup();
    MainUiSetup();
}

void loop(){
    WifiOk();
    IpOk();
    DataLoop();
    lv_timer_handler();
    lv_task_handler();

    if (IsReadyForOtaUpdate())
    {
        OtaDoUpdate();
    }
}