#pragma once

enum {
    PRINTER_STATE_ERROR = 0,
    PRINTER_STATE_IDLE = 1,
    PRINTER_STATE_PRINTING = 2,
};

extern const char* printer_state_messages[];

typedef struct Printer {
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
} _Printer;

extern Printer printer;

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2

void data_loop();
void data_setup();
void send_gcode(bool wait, const char* gcode);