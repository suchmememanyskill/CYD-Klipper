#ifdef CYD_BOARD_JC3248W535C

#include "../screen_driver.h"
#include <databus/Arduino_ESP32QSPI.h>
#include <display/Arduino_AXS15231B.h>
#include <canvas/Arduino_Canvas.h>
#include "lvgl.h"
#include "../lv_setup.h"
#include "../../conf/global_config.h"
#include <Wire.h>
#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

struct TouchPoint {
  uint8_t gesture;
  uint8_t num;
  uint8_t x_h : 4;
  uint8_t _pad1 : 2;
  uint8_t event : 2;
  uint8_t x_l;
  uint8_t y_h : 4;
  uint8_t _pad2 : 4;
  uint8_t y_l;
} __attribute__((packed));

static Arduino_ESP32QSPI qspiBus(
    LCD_CS, LCD_CLK,
    LCD_D0, LCD_D1,
    LCD_D2, LCD_D3,
    false);


Arduino_GFX *gfx = new Arduino_AXS15231B(
    &qspiBus, -1, 2, true,
    LCD_WIDTH, LCD_HEIGHT,
    0, 0, 0, 0);

#ifdef CYD_SCREEN_VERTICAL
    static bool horizontal = false;
    static Arduino_Canvas canvas(CYD_SCREEN_WIDTH_PX, CYD_SCREEN_HEIGHT_PX, gfx, 0, 0);
#else
    static bool horizontal = true;
    static Arduino_Canvas canvas(CYD_SCREEN_HEIGHT_PX, CYD_SCREEN_WIDTH_PX, gfx, 0, 0);

#endif

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf = nullptr;
static lv_disp_t *main_disp = nullptr;


void screen_setBrightness(uint8_t brightness)
{
    uint32_t duty = (4095UL * brightness) / 255UL;
    ledcWrite(0, duty);
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    int x = area->x1;
    int y = area->y1;
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    canvas.draw16bitRGBBitmap(x, y, (uint16_t *)color_p, w, h);
    canvas.flush();

    lv_disp_flush_ready(disp);
}

// Reads 1 touch point from AXS15231B controller at I2C addr 0x3B
bool read_touch(uint16_t &x, uint16_t &y) {
  const uint8_t addr = 0x3B;

  const uint8_t read_cmd[11] = {
    0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00,
    0x00, 8, // length of expected reply (MSB, LSB)
    0x00, 0x00, 0x00
  };

  Wire.beginTransmission(addr);
  Wire.write(read_cmd, sizeof(read_cmd));
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  uint8_t data[8] = {0};
  if (Wire.requestFrom(addr, (uint8_t)sizeof(data)) != sizeof(data)) {
    return false;
  }

  for (uint8_t i = 0; i < sizeof(data); i++) {
    data[i] = Wire.read();
  }

  TouchPoint *p = (TouchPoint *)data;

  if (p->num > 0 && p->num <= 2) {
    x = ((p->x_h & 0x0F) << 8) | p->x_l;
    y = ((p->y_h & 0x0F) << 8) | p->y_l;

    // Clamp to screen bounds
    if (x >= CYD_SCREEN_WIDTH_PX) x = CYD_SCREEN_WIDTH_PX - 1;
    if (y >= CYD_SCREEN_HEIGHT_PX) y = CYD_SCREEN_HEIGHT_PX - 1;
    return true;
  }

  return false;
}


void screen_lv_touchRead(lv_indev_drv_t * /*indev_driver*/, lv_indev_data_t *data)
{
  uint16_t x, y;
  if (read_touch(x, y)) {
    x = min(x, uint16_t(CYD_SCREEN_WIDTH_PX - 1));
    y = min(y, uint16_t(CYD_SCREEN_HEIGHT_PX - 1));
    // Adjust coordinates based on screen rotation
    if (global_config.rotate_screen) {
      // Assuming rotation = 2 (180 degrees)
      x = CYD_SCREEN_WIDTH_PX - x;
      y = CYD_SCREEN_HEIGHT_PX - y;
    }

    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void set_invert_display()
{
    gfx->invertDisplay(global_config.printer_config[global_config.printer_index].invert_colors);
}

void screen_setup()
{
    pinMode(LCD_BL_PIN, OUTPUT);
    ledcSetup(0, 5000, 12);
    ledcAttachPin(LCD_BL_PIN, 0);
    screen_setBrightness(255);

    // gfx->begin();
    canvas.begin();
    gfx->invertDisplay(true);  // OK after begin()
    gfx->setRotation(global_config.rotate_screen ? 2 : 0);
    canvas.fillScreen(0x0000);
    canvas.flush();

    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    lv_init();

    // Allocate full canvas buffer for LVGL
    buf = (lv_color_t *)heap_caps_malloc(
        CYD_SCREEN_WIDTH_PX * CYD_SCREEN_HEIGHT_PX * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    assert(buf);

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, CYD_SCREEN_WIDTH_PX * CYD_SCREEN_HEIGHT_PX);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    
    disp_drv.hor_res = CYD_SCREEN_WIDTH_PX;
    disp_drv.ver_res = CYD_SCREEN_HEIGHT_PX;
    if(horizontal){
        main_disp = lv_disp_drv_register(&disp_drv);
        lv_disp_set_rotation(main_disp, LV_DISP_ROT_90);
    } else {
        lv_disp_drv_register(&disp_drv);
    }
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);
}   

#endif // CYD_BOARD_JC3248W535C
