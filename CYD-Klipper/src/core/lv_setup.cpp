#include "lv_setup.h"
#include "screen_driver.h"
#include "../conf/global_config.h"
#include "lvgl.h"
#include "../ui/ui_utils.h"
#include <Esp.h>

#ifndef CPU_FREQ_HIGH
#define CPU_FREQ_HIGH 240
#endif
#ifndef CPU_FREQ_LOW
#define CPU_FREQ_LOW 80
#endif

typedef void (*lv_indev_drv_read_cb_t)(struct _lv_indev_drv_t * indev_driver, lv_indev_data_t * data);

bool isScreenInSleep = false;
lv_timer_t *screenSleepTimer;
lv_coord_t point[2] = {0};

static lv_indev_drv_read_cb_t originalTouchDriver = NULL;

void LvTouchInterceptCalibration(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    originalTouchDriver(indev_driver, data);

    if (data->state == LV_INDEV_STATE_PR){
        point[0] = data->point.x;
        point[1] = data->point.y;

        while (data->state == LV_INDEV_STATE_PR){
            originalTouchDriver(indev_driver, data);
            delay(20);
        }
    }

    data->state = LV_INDEV_STATE_REL;
}
bool IsScreenAsleep()
{
    return isScreenInSleep;
}

void SetScreenBrightness()
{
    if (globalConfig.brightness < 32)
        SetScreenBrightness(255);
    else
        SetScreenBrightness(globalConfig.brightness);
}


void ScreenTimerSleep(lv_timer_t *timer)
{
#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    SetScreenBrightness(0);
    isScreenInSleep = true;

    // Screen is off, no need to make the cpu run fast, the user won't notice ;)
    setCpuFrequencyMhz(CPU_FREQ_LOW);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
#endif
}

void ScreenTimerSetup()
{
    screenSleepTimer = lv_timer_create(ScreenTimerSleep, globalConfig.screenTimeout * 1000 * 60, NULL);
    lv_timer_pause(screenSleepTimer);
}

void ScreenTimerStart()
{
    lv_timer_resume(screenSleepTimer);
}

void ScreenTimerStop()
{
    lv_timer_pause(screenSleepTimer);
}

void ScreenTimerPeriod(unsigned int period)
{
    lv_timer_set_period(screenSleepTimer, period);
}

void SetScreenTimerPeriod()
{
    ScreenTimerPeriod(globalConfig.screenTimeout * 1000 * 60);
}


void LvDoCalibration(){
    if (globalConfig.screenCalibrated){
        return;
    }

    lv_indev_t * displayDriver = lv_indev_get_next(NULL);
    displayDriver->driver->read_cb = LvTouchInterceptCalibration;

    lv_obj_clean(lv_scr_act());
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Calibrate Screen");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * line = lv_line_create(lv_scr_act());
    static lv_point_t line_points_x[] = { {0, 10}, {21, 10} };
    static lv_point_t line_points_y[] = { {10, 0}, {10, 21} };

    lv_line_set_points(line, line_points_x, 2);
    lv_obj_align(line, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_line_width(line, 1, 0);

    lv_obj_t * line2 = lv_line_create(lv_scr_act());
    lv_line_set_points(line2, line_points_y, 2);
    lv_obj_align(line2, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_line_width(line2, 1, 0);

    while (true){
        lv_timer_handler();
        lv_task_handler();

        if (point[0] != 0 && point[1] != 0){
            break;
        }
    }

    lv_coord_t x1 = point[0];
    lv_coord_t y1 = point[1];
    point[0] = 0;
    point[1] = 0;

    lv_obj_del(line);
    lv_obj_del(line2);

    line = lv_line_create(lv_scr_act());
    lv_line_set_points(line, line_points_x, 2);
    lv_obj_align(line, LV_ALIGN_BOTTOM_RIGHT, 1, -10);
    lv_obj_set_style_line_width(line, 1, 0);

    line = lv_line_create(lv_scr_act());
    lv_line_set_points(line, line_points_y, 2);
    lv_obj_align(line, LV_ALIGN_BOTTOM_RIGHT, -10, 1);

    while (true){
        lv_timer_handler();
        lv_task_handler();

        if (point[0] != 0 && point[1] != 0){
            break;
        }
    }

    lv_coord_t x2 = point[0];
    lv_coord_t y2 = point[1];

    int16_t xDist = CYD_SCREEN_WIDTH_PX - 20;
    int16_t yDist = CYD_SCREEN_HEIGHT_PX - 20;

    globalConfig.screenCalXMult = (float)xDist / (float)(x2 - x1);
    globalConfig.screenCalXOffset = 10.0 - ((float)x1 * globalConfig.screenCalXMult);

    globalConfig.screenCalYMult = (float)yDist / (float)(y2 - y1);
    globalConfig.screenCalYOffset = 10.0 - ((float)y1 * globalConfig.screenCalYMult);

    globalConfig.screenCalibrated = true;
    WriteGlobalConfig();

    lv_obj_clean(lv_scr_act());
}

void SetColorScheme()
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_color_t mainColor = {0};
    ColorDef colorDef = colorDefs[globalConfig.colorScheme];

    if (colorDefs[globalConfig.colorScheme].primaryColorLight > 0){
        mainColor = lv_palette_lighten(colorDef.primaryColor, colorDef.primaryColorLight);
    }
    else if (colorDefs[globalConfig.colorScheme].primaryColorLight < 0) {
        mainColor = lv_palette_darken(colorDef.primaryColor, colorDef.primaryColorLight * -1);
    }
    else {
        mainColor = lv_palette_main(colorDefs[globalConfig.colorScheme].primaryColor);
    }

    lv_theme_t *theme = lv_theme_default_init(dispp, mainColor, lv_palette_main(colorDef.secondaryColor), !globalConfig.lightMode, &CYD_SCREEN_FONT);
    lv_disp_set_theme(dispp, theme);
}

void ScreenTimerWake()
{
#ifndef CYD_SCREEN_DISABLE_TIMEOUT
    lv_timer_reset(screenSleepTimer);

    if (!isScreenInSleep){
        return;
    }

    isScreenInSleep = false;
    SetScreenBrightness();

    // Reset cpu freq
    setCpuFrequencyMhz(CPU_FREQ_HIGH);
    Serial.printf("CPU Speed: %d MHz\n", ESP.getCpuFreqMHz());
#endif
}


void LvTouchIntercept(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    originalTouchDriver(indev_driver, data);

    if (data->state == LV_INDEV_STATE_PR) {
        if (IsScreenAsleep()) {
            while (data->state == LV_INDEV_STATE_PR) {
                originalTouchDriver(indev_driver, data);
                delay(20);
            }

            data->state = LV_INDEV_STATE_REL;
        }

        ScreenTimerWake();
#ifndef CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
        data->point.x = round((data->point.x * globalConfig.screenCalXMult) + globalConfig.screenCalXOffset);
        data->point.y = round((data->point.y * globalConfig.screenCalYMult) + globalConfig.screenCalYOffset);
#endif // CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
    }
}

void LvSetup()
{
    lv_indev_t * displayDriver = lv_indev_get_next(NULL);

    if (originalTouchDriver == NULL)
    {
        originalTouchDriver = displayDriver->driver->read_cb;
    }

    SetColorScheme();

#ifndef CYD_SCREEN_DISABLE_TOUCH_CALIBRATION
    LvDoCalibration();
#endif // CYD_SCREEN_DISABLE_TOUCH_CALIBRATION

    displayDriver->driver->read_cb = LvTouchIntercept;

    ScreenTimerSetup();
    ScreenTimerStart();
}
