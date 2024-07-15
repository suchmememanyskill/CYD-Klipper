#include "../lib/ESP32OTAPull.h"
#include "lvgl.h"
#include "ui_utils.h"
#include "../core/lv_setup.h"
#include "../core/data_setup.h"
#include "../conf/global_config.h"
#include "ota_setup.h"
#include "../core/macros_query.h"
#include "../core/files_query.h"
#include "gcode_img.h"

//const char *ota_url = "https://gist.githubusercontent.com/suchmememanyskill/ece418fe199e155340de6c224a0badf2/raw/0d6762d68bc807cbecc71e40d55b76692397a7b3/update.json"; // Test url
const char *ota_url = "https://suchmememanyskill.github.io/CYD-Klipper/OTA.json"; // Prod url
ESP32OTAPull ota_pull;
static bool update_available;
static bool ready_for_ota_update = false;

String ota_new_version_name()
{
    return ota_pull.GetVersion();
}

bool ota_has_update()
{
    return update_available;
}

static int last_callback_time = 0;
lv_obj_t *percentage_bar;
lv_obj_t *update_label;
void do_update_callback(int offset, int totallength)
{
    int now = millis();
    if (now - last_callback_time < 1000)
    {
        return;
    }

    last_callback_time = now;

    float percentage = (float)offset / (float)totallength; // 0 -> 1
    lv_bar_set_value(percentage_bar, percentage * 100, LV_ANIM_OFF);
    lv_label_set_text_fmt(update_label, "%d/%d bytes", offset, totallength);

    lv_refr_now(NULL);
    lv_timer_handler();
    lv_task_handler();
}

void ota_do_update(bool variant_automatic)
{
    LOG_LN("Starting OTA Update");
    lv_obj_clean(lv_scr_act());

    lv_obj_t *panel = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_size(panel, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_layout_flex_column(panel, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, "Updating OTA...");

    percentage_bar = lv_bar_create(panel);
    lv_obj_set_size(percentage_bar, CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 3, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * 0.75f);

    update_label = lv_label_create(panel);
    lv_label_set_text(update_label, "0/0");

    if (!variant_automatic) {
        LOG_LN("Freezing Background Tasks");
        screen_timer_wake();
        screen_timer_stop();
        freeze_request_thread();
    }

    lv_refr_now(NULL);
    lv_timer_handler();
    lv_task_handler();

    macros_clear();
    power_devices_clear();
    clear_files();
    clear_img_mem();

    ota_pull.SetCallback(do_update_callback);
    ota_pull.CheckForOTAUpdate(ota_url, REPO_VERSION, ESP32OTAPull::ActionType::UPDATE_AND_BOOT);
}

void ota_init()
{
    //ota_pull.AllowDowngrades(true);
    int result = ota_pull.CheckForOTAUpdate(ota_url, REPO_VERSION, ESP32OTAPull::ActionType::DONT_DO_UPDATE);
    LOG_F(("OTA Update Result: %d\n", result))
    update_available = result == ESP32OTAPull::UPDATE_AVAILABLE;

    if (global_config.auto_ota_update && update_available)
    {
        ota_do_update(true);
    }
}

void set_ready_for_ota_update()
{
    ready_for_ota_update = true;
}

bool is_ready_for_ota_update()
{
    return ready_for_ota_update;
}

