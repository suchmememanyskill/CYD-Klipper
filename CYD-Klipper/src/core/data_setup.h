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
    char* print_filename; 
    float print_progress; // 0 -> 1
    float fan_speed; // 0 -> 1
    float gcode_offset[3];
    float speed_mult;
    float extrude_mult;
    int total_layers;
    int current_layer;
    float pressure_advance;
    float smooth_time;
    int feedrate_mm_per_s;
    int slicer_estimated_print_time_s;
} Printer;

extern Printer printer;
extern int klipper_request_consecutive_fail_count;

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2
#define DATA_PRINTER_TEMP_PRESET 3

void data_loop();
void data_setup();
void send_gcode(bool wait, const char* gcode);
void move_printer(const char* axis, float amount, bool relative);

void freeze_request_thread();
void unfreeze_request_thread();