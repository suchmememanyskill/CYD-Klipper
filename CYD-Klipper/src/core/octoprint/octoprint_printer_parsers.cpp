#include "../printer_integration.hpp"
#include "octoprint_printer_integration.hpp"
#include <ArduinoJson.h>

void OctoPrinter::parse_printer_state(JsonDocument& in)
{
    auto flags = in["state"]["flags"];
    auto text = in["state"]["text"];

    bool cancelling = flags["cancelling"];
    bool closedOrError = flags["closedOrError"];
    bool error = flags["error"];
    bool finishing = flags["finishing"];
    bool operational = flags["operational"];
    bool paused = flags["paused"];
    bool pausing = flags["pausing"];
    bool printing = flags["printing"];
    bool ready = flags["ready"];
    bool resuming = flags["resuming"];
    bool sdReady = flags["sdReady"];

    if (printing || resuming)
    {
        printer_data.state = PrinterState::PrinterStatePrinting;
    }
    else if (pausing || paused)
    {
        printer_data.state = PrinterState::PrinterStatePaused;
    }
    else if (cancelling || finishing || ready)
    {
        printer_data.state = PrinterState::PrinterStateIdle;
    }
    else
    {
        if (text != NULL && (printer_data.state_message == NULL || strcmp(printer_data.state_message, text)))
        {
            printer_data.state_message = (char *)malloc(strlen(text) + 1);
            strcpy(printer_data.state_message, text);
        }

        printer_data.state = PrinterState::PrinterStateError;
    }

    auto temperature = in["temperature"];

    if (temperature.containsKey("bed"))
    {
        printer_data.temperatures[PrinterTemperatureDeviceIndexBed] = temperature["bed"]["actual"];
        printer_data.target_temperatures[PrinterTemperatureDeviceIndexBed] = temperature["bed"]["target"];
    }

    if (temperature.containsKey("tool0"))
    {
        printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] = temperature["tool0"]["actual"];
        printer_data.target_temperatures[PrinterTemperatureDeviceIndexNozzle1] = temperature["tool0"]["target"];
    }

    printer_data.can_extrude = printer_data.temperatures[PrinterTemperatureDeviceIndexNozzle1] >= MIN_EXTRUDER_EXTRUDE_TEMP;
    printer_data.homed_axis = true;
}

void OctoPrinter::parse_job_state(JsonDocument& in)
{
    auto job = in["job"];

    if (job.containsKey("file"))
    {
        const char* name = job["file"]["name"];

        if (name != NULL && (printer_data.print_filename == NULL || strcmp(printer_data.print_filename, name)))
        {
            printer_data.print_filename = (char *)malloc(strlen(name) + 1);
            strcpy(printer_data.print_filename, name);
        }
    }

    if (job.containsKey("filament") && job["filament"] != NULL && job["filament"].containsKey("tool0"))
    {
        printer_data.filament_used_mm = job["filament"]["tool0"]["length"];
    }

    auto progress = in["progress"];
    float completion = progress["completion"];
    printer_data.print_progress = completion / 100;
    printer_data.elapsed_time_s = progress["printTime"];
    printer_data.printed_time_s = progress["printTime"];
    printer_data.remaining_time_s = progress["printTimeLeft"];
}

void OctoPrinter::parse_error(JsonDocument& in)
{
    const char* error = in["error"];
    if (error != NULL)
    {
        printer_data.state = PrinterState::PrinterStateError;

        if (printer_data.state_message == NULL || strcmp(printer_data.state_message, error))
        {
            printer_data.state_message = (char *)malloc(strlen(error) + 1);
            strcpy(printer_data.state_message, error);
        }
    }
}