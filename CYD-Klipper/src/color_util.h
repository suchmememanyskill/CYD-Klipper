#pragma once

#include "lvgl.h"
#include "conf/global_config.h"
#ifndef COLOR_H
#define COLOR_H

#include "lvgl.h"

class Color {
public:
    static lv_color_t fromHSL(uint16_t hue, uint8_t sat, uint8_t lum);
    static lv_color_t fromRGB(uint8_t red, uint8_t green, uint8_t blue);
    static lv_color_t fromBGR(uint8_t blue, uint8_t green, uint8_t red);
    static lv_color_t fromGRB(uint8_t green, uint8_t red, uint8_t blue);
    static lv_color_t fromHex(uint32_t hex);
    static lv_color_t fromHex3(uint16_t hex);
    static lv_color_t fromLvColor(lv_color_t color);

    static lv_color_t invert(lv_color_t color);
    static lv_color_t lighten(lv_color_t color, float amount);
    static lv_color_t darken(lv_color_t color, float amount);
    static lv_color_t mix(lv_color_t color1, lv_color_t color2, float amount);
    static lv_color_t lerp(lv_color_t color1, lv_color_t color2, float t);
    static lv_color_t slerp(lv_color_t color1, lv_color_t color2, float t);
    static void rgb2hsl(float r, float g, float b, float& h, float& s, float& l); 
    static void hsl2rgb(float h, float s, float l, float& r, float& g, float& b);
    static float hue2rgb(float p, float q, float t);
private:
};

#endif
