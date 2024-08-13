#pragma once

enum {
    PRINTER_STATE_OFFLINE = 0,
    PRINTER_STATE_ERROR = 1,
    PRINTER_STATE_IDLE = 2,
    PRINTER_STATE_PRINTING = 3,
    PRINTER_STATE_PAUSED = 4,
};

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
    float printed_time_s;
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

typedef struct _PrinterMinimal {
    unsigned char state;
    float print_progress; // 0 -> 1
    unsigned int power_devices;
} PrinterMinimal;

extern Printer printer;
extern PrinterMinimal *printer_minimal;
extern int klipper_request_consecutive_fail_count;

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2
#define DATA_PRINTER_TEMP_PRESET 3
#define DATA_PRINTER_MINIMAL 4

void data_loop();
void data_setup();
void send_estop();
void send_gcode(bool wait, const char* gcode);
void move_printer(const char* axis, float amount, bool relative);

void freeze_request_thread();
void unfreeze_request_thread();