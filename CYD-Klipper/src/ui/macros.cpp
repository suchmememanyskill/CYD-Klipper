#include "macros.h"
#include "ui_utils.h"
#include <Esp.h>
#include "../core/current_printer.h"
#include "../core/data_setup.h"
#include "../core/semaphore.h"

typedef struct {
    const char* power_device_name;
    BasePrinter* printer;
} DoubleStorage;

static void macro_run(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    const char* macro = (const char*)lv_event_get_user_data(e);
    LOG_F(("Macro: %s\n", macro))
    current_printer_execute_macro(macro);
}

int macros_add_macros_to_panel(lv_obj_t * root_panel, BasePrinter* printer)
{
    freeze_request_thread();
    Macros macros = printer->get_macros();
    unfreeze_request_thread();

    if (!macros.success)
    {
        return 0;
    }

    if (global_config.sort_macros)
    {
        std::sort(macros.macros, macros.macros + macros.count, [](const char *a, const char *b)
                  { return strcmp(a, b) < 0; });
    }

    for (int i = 0; i < macros.count; i++)
    {
        const char* macro = macros.macros[i];
        lv_obj_on_destroy_free_data(root_panel, macro);
        lv_create_custom_menu_button(macro, root_panel, macro_run, "Run", (void*)macro);
    }

    free(macros.macros);
    return macros.count;
}

static void power_device_toggle(lv_event_t * e)
{
    bool checked = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
    DoubleStorage* device = (DoubleStorage*)lv_event_get_user_data(e);
    LOG_F(("Power Device: %s, State: %d -> %d\n", device->power_device_name, !checked, checked))

    freeze_request_thread();
    device->printer->set_power_device_state(device->power_device_name, checked);
    unfreeze_request_thread();
}

int macros_add_power_devices_to_panel(lv_obj_t * root_panel, BasePrinter* printer)
{
    freeze_request_thread();
    PowerDevices devices = printer->get_power_devices();
    unfreeze_request_thread();

    if (!devices.success)
    {
        return 0;
    }

    for (int i = 0; i < devices.count; i++)
    {
        const char* power_device_name = devices.power_devices[i];
        const bool power_device_state = devices.power_states[i];
        DoubleStorage* storage = (DoubleStorage*)malloc(sizeof(DoubleStorage));
        storage->printer = printer;
        storage->power_device_name = power_device_name;
        lv_obj_on_destroy_free_data(root_panel, storage);
        lv_obj_on_destroy_free_data(root_panel, power_device_name);
        lv_create_custom_menu_switch(power_device_name, root_panel, power_device_toggle, power_device_state, (void*)storage);
    }

    free(devices.power_devices);
    free(devices.power_states);
    return devices.count;
}

void macros_draw_power_fullscreen(BasePrinter* printer)
{
    lv_obj_t * parent = lv_create_empty_panel(lv_scr_act());
    lv_obj_set_style_bg_opa(parent, LV_OPA_100, 0); 
    lv_obj_align(parent, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(parent, CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX);
    lv_layout_flex_column(parent);

    lv_obj_set_size(lv_create_empty_panel(parent), 0, 0);

    auto width = CYD_SCREEN_WIDTH_PX - CYD_SCREEN_GAP_PX * 2;

    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
    lv_obj_add_event_cb(btn, destroy_event_user_data, LV_EVENT_CLICKED, parent);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE " Close");
    lv_obj_center(label);

    macros_add_power_devices_to_panel(parent, printer);
}

void macros_draw_power_fullscreen()
{
    macros_draw_power_fullscreen(get_current_printer());
}