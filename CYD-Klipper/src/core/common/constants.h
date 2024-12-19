#pragma once

extern const char* fan_speeds_col_1[];
extern const int fan_speeds_col_1_values[];

extern const char* fan_speeds_col_2[];
extern const int fan_speeds_col_2_values[];

extern const char* fan_speeds_col_3[];
extern const int fan_speeds_col_3_values[];

unsigned char fan_percent_to_byte(int percent);

#define FAN_SPEED_COLUMN(set_fan_speed, column_name) lv_button_column_t column_name[] = {{ set_fan_speed, fan_speeds_col_2, (const void**)fan_speeds_col_2_values, 4},{ set_fan_speed, fan_speeds_col_3, (const void**)fan_speeds_col_3_values, 4}, { set_fan_speed, fan_speeds_col_1, (const void**)fan_speeds_col_1_values, 2}};