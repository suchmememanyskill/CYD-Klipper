// Adapted from https://github.com/OzInFl/Elecrow-3.5-RGB-TFT-SQUARELINE-EXAMPLE

#ifdef CYD_SCREEN_DRIVER_ESP32_CROWPANEL_70
#include "../screen_driver.h"
#include "lvgl.h"
#include "../../conf/global_config.h"
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <Arduino.h>
#include <Wire.h>

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[CYD_SCREEN_HEIGHT_PX * CYD_SCREEN_WIDTH_PX / 10];

class LGFX : public lgfx::LGFX_Device{

    public:
    
        lgfx::Bus_RGB     _bus_instance;
        lgfx::Panel_RGB   _panel_instance;
    
        LGFX(void)
        {
    
    
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;
            
            cfg.pin_d0  = GPIO_NUM_15; // B0
            cfg.pin_d1  = GPIO_NUM_7;  // B1
            cfg.pin_d2  = GPIO_NUM_6;  // B2
            cfg.pin_d3  = GPIO_NUM_5;  // B3
            cfg.pin_d4  = GPIO_NUM_4;  // B4
            
            cfg.pin_d5  = GPIO_NUM_9;  // G0
            cfg.pin_d6  = GPIO_NUM_46; // G1
            cfg.pin_d7  = GPIO_NUM_3;  // G2
            cfg.pin_d8  = GPIO_NUM_8;  // G3
            cfg.pin_d9  = GPIO_NUM_16; // G4
            cfg.pin_d10 = GPIO_NUM_1;  // G5
            
            cfg.pin_d11 = GPIO_NUM_14; // R0
            cfg.pin_d12 = GPIO_NUM_21; // R1
            cfg.pin_d13 = GPIO_NUM_47; // R2
            cfg.pin_d14 = GPIO_NUM_48; // R3
            cfg.pin_d15 = GPIO_NUM_45; // R4
    
            cfg.pin_henable = GPIO_NUM_41;
            cfg.pin_vsync   = GPIO_NUM_40;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_0;
            cfg.freq_write  = 15000000;
    
            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 40;
            cfg.hsync_pulse_width = 48;
            cfg.hsync_back_porch  = 40;
            
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 1;
            cfg.vsync_pulse_width = 31;
            cfg.vsync_back_porch  = 13;
    
            cfg.pclk_active_neg   = 1;
            cfg.de_idle_high      = 0;
            cfg.pclk_idle_high    = 0;
    
            _bus_instance.config(cfg);
        }
                {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = 800;
            cfg.memory_height = 480;
            cfg.panel_width  = 800;
            cfg.panel_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);
        setPanel(&_panel_instance);
        }
    };

LGFX tft;


#define TFT_BL 2



// Touch setup 
#define TOUCH_GT911
#define TOUCH_GT911_SCL 20//20
#define TOUCH_GT911_SDA 19//19
#define TOUCH_GT911_INT -1//-1
#define TOUCH_GT911_RST -1//38
#define TOUCH_GT911_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 800//480
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480//272
#define TOUCH_MAP_Y2 0

#include <TAMC_GT911.h>
TAMC_GT911 ts = TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));
int touch_last_x = 0, touch_last_y = 0;

void touch_init()
{
  Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
  ts.begin();
  ts.setRotation(TOUCH_GT911_ROTATION);
}

bool touch_has_signal()
{
  return true;
}
class LGFX : public lgfx::LGFX_Device{

    public:
    
        lgfx::Bus_RGB     _bus_instance;
        lgfx::Panel_RGB   _panel_instance;
    
        LGFX(void)
        {
    
    
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;
            
            cfg.pin_d0  = GPIO_NUM_15; // B0
            cfg.pin_d1  = GPIO_NUM_7;  // B1
            cfg.pin_d2  = GPIO_NUM_6;  // B2
            cfg.pin_d3  = GPIO_NUM_5;  // B3
            cfg.pin_d4  = GPIO_NUM_4;  // B4
            
            cfg.pin_d5  = GPIO_NUM_9;  // G0
            cfg.pin_d6  = GPIO_NUM_46; // G1
            cfg.pin_d7  = GPIO_NUM_3;  // G2
            cfg.pin_d8  = GPIO_NUM_8;  // G3
            cfg.pin_d9  = GPIO_NUM_16; // G4
            cfg.pin_d10 = GPIO_NUM_1;  // G5
            
            cfg.pin_d11 = GPIO_NUM_14; // R0
            cfg.pin_d12 = GPIO_NUM_21; // R1
            cfg.pin_d13 = GPIO_NUM_47; // R2
            cfg.pin_d14 = GPIO_NUM_48; // R3
            cfg.pin_d15 = GPIO_NUM_45; // R4
    
            cfg.pin_henable = GPIO_NUM_41;
            cfg.pin_vsync   = GPIO_NUM_40;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_0;
            cfg.freq_write  = 15000000;
    
            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 40;
            cfg.hsync_pulse_width = 48;
            cfg.hsync_back_porch  = 40;
            
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 1;
            cfg.vsync_pulse_width = 31;
            cfg.vsync_back_porch  = 13;
    
            cfg.pclk_active_neg   = 1;
            cfg.de_idle_high      = 0;
            cfg.pclk_idle_high    = 0;
    
            _bus_instance.config(cfg);
        }
                {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = 800;
            cfg.memory_height = 480;
            cfg.panel_width  = 800;
            cfg.panel_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);
        setPanel(&_panel_instance);
        }
    }
bool touch_touched()
{
  ts.read();
  if (ts.isTouched)
  {
    touch_last_x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, tft.width() - 1);
    touch_last_y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, tft.height() - 1);
    return true;
  }
  else
  {
    return false;
  }
}

bool touch_released()
{
  return true;
}

// screen driver

void screen_setBrightness(unsigned char brightness)
{
    ledcWrite(1, brightness); /* Screen brightness can be modified by adjusting this parameter. (0-255) */  
}

void set_invert_display()
{
    // TODO
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
  
    tft.pushImageDMA(area->x1, area->y1, w, h,(lgfx::rgb565_t*)&color_p->full);//
    lv_disp_flush_ready(disp);
}

void screen_lv_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (touch_has_signal())
    {
      if (touch_touched())
      {
        data->state = LV_INDEV_STATE_PR;
  
        /*Set the coordinates*/
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
        //Serial.print( "Data x " );
        //Serial.println( data->point.x );
        //Serial.print( "Data y " );
        //Serial.println( data->point.y );
      }
      else if (touch_released())
      {
        data->state = LV_INDEV_STATE_REL;
      }
    }
    else
    {
      data->state = LV_INDEV_STATE_REL;
    }
}

void screen_setup()
{

    tft.begin();
    //tft.setRotation(global_config.rotate_screen ? 3 : 1);

    delay(500);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    ledcSetup(1, 300, 8);
    ledcAttachPin(TFT_BL, 1);

    screen_setBrightness(255);

    touch_init();

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, CYD_SCREEN_WIDTH_PX * CYD_SCREEN_HEIGHT_PX / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = CYD_SCREEN_WIDTH_PX;
    disp_drv.ver_res = CYD_SCREEN_HEIGHT_PX;
    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);
}

#endif