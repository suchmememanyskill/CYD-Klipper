#include "constants.h"

const char* fan_speeds_col_1[] = { "On", "Off" };
const int fan_speeds_col_1_values[] = { 0, 100 };

const char* fan_speeds_col_2[] = { "10%", "20%", "30%", "40%"};
const int fan_speeds_col_2_values[] = { 10, 20, 30, 40 };

const char* fan_speeds_col_3[] = { "50%", "60%", "70%", "80%"};
const int fan_speeds_col_3_values[] = { 50, 60, 70, 80 };

char fan_percent_to_byte(int percent)
{
    return percent * 255 / 100;
}