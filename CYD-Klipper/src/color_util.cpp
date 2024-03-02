// #pragma once

// #include "color_util.h"
// #include <cmath>

// lv_color_t Color::fromHSL(uint16_t hue, uint8_t sat, uint8_t lum) {
//     float h = hue / 360.0f;
//     float s = sat / 100.0f;
//     float l = lum / 100.0f;

//     float c = (1 - std::abs(2 * l - 1)) * s;
//     float x = c * (1 - std::abs(std::fmod(h * 6, 2) - 1));
//     float m = l - c / 2;

//     float r, g, b;

//     if (h * 6 < 1) {
//         r = c;
//         g = x;
//         b = 0;
//     } else if (h * 6 < 2) {
//         r = x;
//         g = c;
//         b = 0;
//     } else if (h * 6 < 3) {
//         r = 0;
//         g = c;
//         b = x;
//     } else if (h * 6 < 4) {
//         r = 0;
//         g = x;
//         b = c;
//     } else if (h * 6 < 5) {
//         r = x;
//         g = 0;
//         b = c;
//     } else {
//         r = c;
//         g = 0;
//         b = x;
//     }

//     r += m;
//     g += m;
//     b += m;

//     r *= 255;
//     g *= 255;
//     b *= 255;

//     lv_color_t color;
//     color.ch.red = r;
//     color.ch.green = g;
//     color.ch.blue = b;

//     return color;
// }

// lv_color_t Color::fromRGB(uint8_t red, uint8_t green, uint8_t blue) {
//     lv_color_t color;
//     color.ch.red = red;
//     color.ch.green = green;
//     color.ch.blue = blue;
//     return color;
// }

// lv_color_t Color::fromBGR(uint8_t blue, uint8_t green, uint8_t red) {
//     lv_color_t color;
//     color.ch.red = red;
//     color.ch.green = green;
//     color.ch.blue = blue;
//     return color;
// }

// lv_color_t Color::fromGRB(uint8_t green, uint8_t red, uint8_t blue) {
//     lv_color_t color;
//     color.ch.red = red;
//     color.ch.green = green;
//     color.ch.blue = blue;
//     return color;
// }

// lv_color_t Color::fromHex(uint32_t hex) {
//     uint8_t red = (hex >> 16) & 0xFF;
//     uint8_t green = (hex >> 8) & 0xFF;
//     uint8_t blue = hex & 0xFF;
    
    
//     return lv_color_make(red, green, blue);
// }

// lv_color_t Color::fromHex3(uint16_t hex) {
//     uint8_t red = ((hex >> 8) & 0xF) * 17;
//     uint8_t green = ((hex >> 4) & 0xF) * 17;
//     uint8_t blue = (hex & 0xF) * 17;
//     lv_color_t color;
//     color.ch.red = red;
//     color.ch.green = green;
//     color.ch.blue = blue;
//     return color;
// }

// lv_color_t Color::fromLvColor(lv_color_t color) {
//     return color;
// }

// lv_color_t Color::invert(lv_color_t color) {
//     lv_color_t inv_color;
//     inv_color.ch.red = 255 - color.ch.red;
//     inv_color.ch.green = 255 - color.ch.green;
//     inv_color.ch.blue = 255 - color.ch.blue;
//     return inv_color;
// }

// lv_color_t Color::lighten(lv_color_t color, float amount) {
//     float r = color.ch.red / 255.0f;
//     float g = color.ch.green / 255.0f;
//     float b = color.ch.blue / 255.0f;

//     r += amount;
//     g += amount;
//     b += amount;

//     if (r > 1) r = 1;
//     if (g > 1) g = 1;
//     if (b > 1) b = 1;

//     lv_color_t light_color;
//     light_color.ch.red = r * 255;
//     light_color.ch.green = g * 255;
//     light_color.ch.blue = b * 255;

//     return light_color;
// }

// lv_color_t Color::darken(lv_color_t color, float amount) {
//     float r = color.ch.red / 255.0f;
//     float g = color.ch.green / 255.0f;
//     float b = color.ch.blue / 255.0f;

//     r -= amount;
//     g -= amount;
//     b -= amount;

//     if (r < 0) r = 0;
//     if (g < 0) g = 0;
//     if (b < 0) b = 0;

//     lv_color_t dark_color;
//     dark_color.ch.red = r * 255;
//     dark_color.ch.green = g * 255;
//     dark_color.ch.blue = b * 255;

//     return dark_color;
// }

// lv_color_t Color::mix(lv_color_t color1, lv_color_t color2, float amount) {
//     float r = color1.ch.red * (1 - amount) + color2.ch.red * amount;
//     float g = color1.ch.green * (1 - amount) + color2.ch.green * amount;
//     float b = color1.ch.blue * (1 - amount) + color2.ch.blue * amount;

//     lv_color_t mixed_color;
//     mixed_color.ch.red = r;
//     mixed_color.ch.green = g;
//     mixed_color.ch.blue = b;

//     return mixed_color;
// }

// lv_color_t Color::lerp(lv_color_t color1, lv_color_t color2, float t) {
//     float r = color1.ch.red + t * (color2.ch.red - color1.ch.red);
//     float g = color1.ch.green + t * (color2.ch.green - color1.ch.green);
//     float b = color1.ch.blue + t * (color2.ch.blue - color1.ch.blue);

//     lv_color_t lerped_color;
//     lerped_color.ch.red = r;
//     lerped_color.ch.green = g;
//     lerped_color.ch.blue = b;

//     return lerped_color;
// }

// lv_color_t Color::slerp(lv_color_t color1, lv_color_t color2, float t) {
//     // Convert colors to HSL
//     float h1, s1, l1;
//     rgb2hsl(color1.ch.red, color1.ch.green, color1.ch.blue, h1, s1, l1);
//     float h2, s2, l2;
//     rgb2hsl(color2.ch.red, color2.ch.green, color2.ch.blue, h2, s2, l2);

//     // Interpolate HSL values
//     float h = h1 + t * (h2 - h1);
//     float s = s1 + t * (s2 - s1);
//     float l = l1 + t * (l2 - l1);

//     // Convert interpolated HSL value to RGB
//     float r, g, b;
//     hsl2rgb(h, s, l, r, g, b);

//     lv_color_t slerped_color;
//     slerped_color.ch.red = r * 255;
//     slerped_color.ch.green = g * 255;
//     slerped_color.ch.blue = b * 255;

//     return slerped_color;
// }

// void Color::rgb2hsl(float r, float g, float b, float& h, float& s, float& l) {
//     r /= 255.0f;
//     g /= 255.0f;
//     b /= 255.0f;

//     float min = std::min(r, std::min(g, b));
//     float max = std::max(r, std::max(g, b));

//     l = (min + max) / 2;

//     if (min == max) {
//         h = 0;
//         s = 0;
//     } else {
//         float d = max - min;
//         s = l > 0.5f ? d / (2 - max - min) : d / (max + min);

//         if (max == r) {
//             h = (g - b) / d + (g < b ? 6 : 0);
//         } else if (max == g) {
//             h = (b - r) / d + 2;
//         } else {
//             h = (r - g) / d + 4;
//         }

//         h /= 6;
//     }
// }

// void Color::hsl2rgb(float h, float s, float l, float& r, float& g, float& b) {
//     if (s == 0) {
//         r = g = b = l;
//     } else {
//         float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
//         float p = 2 * l - q;

//         r = hue2rgb(p, q, h + 1 / 3);
//         g = hue2rgb(p, q, h);
//         b = hue2rgb(p, q, h - 1 / 3);
//     }

//     r *= 255;
//     g *= 255;
//     b *= 255;
// }

// float Color::hue2rgb(float p, float q, float t) {
//     if (t < 0) t += 1;
//     if (t > 1) t -= 1;

//     if (t < 1 / 6) return p + (q - p) * 6 * t;
//     if (t < 1 / 2) return q;
//     if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;

//     return p;
// }
