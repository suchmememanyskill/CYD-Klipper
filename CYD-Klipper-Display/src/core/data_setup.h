#pragma once

enum {
    PRINTER_STATE_ERROR = 0,
    PRINTER_STATE_IDLE = 1,
    PRINTER_SATE_PRINTING = 2,
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
} _Printer;

extern Printer printer;

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2

void data_loop();
void data_setup();