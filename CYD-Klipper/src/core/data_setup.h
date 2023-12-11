#pragma once

enum {
    PRINTER_STATE_ERROR = 0,
    PRINTER_STATE_IDLE = 1,
    PRINTER_STATE_PRINTING = 2,
    PRINTER_STATE_PAUSED = 3,
};

extern const char* printer_state_messages[];

typedef struct _Printer {
    unsigned char state;
    char* state_message;
    float extruder_temp;
    float extruder_target_temp;
    float bed_temp;
    float bed_target_temp;
    float position[3];
    unsigned char can_extrude;
    unsigned char homed_axis;
    unsigned char absolute_coords;
    float elapsed_time_s;
    float remaining_time_s;
    float filament_used_mm;
    char* print_filename; // 0 -> 1
    float print_progress;
} Printer;

extern Printer printer;
extern int klipper_request_consecutive_fail_count;

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2
#define DATA_PRINTER_TEMP_PRESET 3

void data_loop();
void data_setup();
void send_gcode(bool wait, const char* gcode);