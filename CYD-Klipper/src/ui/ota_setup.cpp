#include "../lib/ESP32OTAPull.h"
#include "lvgl.h"
#include "ui_utils.h"
#include "../core/lv_setup.h"
#include "../core/data_setup.h"
#include "../conf/global_config.h"
#include "ota_setup.h"

const char *OTA_URL = "https://suchmememanyskill.github.io/CYD-Klipper/OTA.json"; // Prod url
Esp32OtaPull otaPull;
static bool updateAvailable;
static bool readyForOtaUpdate = false;

String OtaNewVersionName()
{
    return otaPull.GetVersion();
}

bool OtaHasUpdate()
{
    return updateAvailable;
}

static int lastCallbackTime = 0;
lv_obj_t *percentageBar;
lv_obj_t *updateLabel;
void DoUpdateCallback(int offset, int totallength)
{
    int now = millis();
    if (now - lastCallbackTime < 1000)
    {
        return;
    }

    lastCallbackTime = now;

    float percentage = (float)offset / (float)totallength; // 0 -> 1
    lv_bar_set_value(percentageBar, percentage * 100, LV_ANIM_OFF);
    lv_label_set_text_fmt(updateLabel, "%d/%d bytes", offset, totallength);

    lv_refr_now(NULL);
    lv_timer_handler();
    lv_task_handler();
}

void OtaDoUpdate(bool variantAutomatic)
{
    Serial.println("Starting OTA Update");
    lv_obj_clean(lv_scr_act());

    lv_obj_t *panel = CreateEmptyPanel(lv_scr_act());
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, 0, 0);
    LayoutFlexColumn(panel, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, "Updating OTA...");

    percentageBar = lv_bar_create(panel);
    lv_obj_set_size(percentageBar, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * 0.75f);

    updateLabel = lv_label_create(panel);
    lv_label_set_text(updateLabel, "0/0");

    if (!variantAutomatic) {
        Serial.println("Freezing Background Tasks");
        ScreenTimerWake();
        ScreenTimerStop();
        FreezeRequestThread();
    }

    lv_refr_now(NULL);
    lv_timer_handler();
    lv_task_handler();

    otaPull.SetCallback(DoUpdateCallback);
    otaPull.CheckForOtaUpdate(OTA_URL, REPO_VERSION, Esp32OtaPull::ActionType::UPDATE_AND_BOOT);
}

void OtaInit()
{
    //ota_pull.AllowDowngrades(true);
    int result = otaPull.CheckForOtaUpdate(OTA_URL, REPO_VERSION, Esp32OtaPull::ActionType::DONT_DO_UPDATE);
    Serial.printf("OTA Update Result: %d\n", result);
    updateAvailable = result == Esp32OtaPull::UPDATE_AVAILABLE;

    if (globalConfig.autoOtaUpdate && updateAvailable)
    {
        OtaDoUpdate(true);
    }
}

void SetReadyForOtaUpdate()
{
    readyForOtaUpdate = true;
}

bool IsReadyForOtaUpdate()
{
    return readyForOtaUpdate;
}
