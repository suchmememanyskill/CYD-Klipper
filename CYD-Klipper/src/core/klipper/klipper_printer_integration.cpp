#include "klipper_printer_integration.hpp"
#include "../../conf/global_config.h"
#include <HTTPClient.h>
#include <UrlEncode.h>

void configure_http_client(HTTPClient &client, String url_part, bool stream, int timeout, PRINTER_CONFIG* printer)
{
    if (stream){
        client.useHTTP10(true);
    }

    if (timeout > 0){
        client.setTimeout(timeout);
        client.setConnectTimeout(timeout);
    }

    client.begin("http://" + String(printer->klipper_host) + ":" + String(printer->klipper_port) + url_part);

    if (printer->auth_configured) {
        client.addHeader("X-Api-Key", printer->klipper_auth);
    }
}

bool KlipperPrinter::send_gcode(const char *gcode, bool wait)
{
    HTTPClient client;
    configure_http_client(client, "/printer/gcode/script?script=" + urlEncode(gcode), false, wait ? 5000 : 750, printer_config);
    LOG_F(("Sending gcode: %s\n", gcode))

    try
    {
        client.GET();
        return true;
    }
    catch (...)
    {
        LOG_LN("Failed to send gcode");
        return false;
    }
}

bool KlipperPrinter::move_printer(const char* axis, float amount, bool relative)
{
    if (!printer_data.homed_axis || printer_data.state == PrinterStatePrinting)
        return true;

    char gcode[64];
    const char* extra = (amount > 0) ? "+" : "";
    const char* start = "";
    const char* end = "";

    if (printer_data.absolute_coords && relative) {
        start = "G91\n";
    }
    else if (!printer_data.absolute_coords && !relative) {
        start = "G90\n";
    }

    if (printer_data.absolute_coords && relative) {
        end = "\nG90";
    }
    else if (!printer_data.absolute_coords && !relative) {
        end = "\nG91";
    }

    sprintf(gcode, "%sG1 %s%s%.3f F6000%s", start, axis, extra, amount, end);
    send_gcode(gcode);

    lock_absolute_relative_mode_swap = 2;
}

bool KlipperPrinter::execute_feature(PrinterFeatures feature)
{
    HTTPClient client;

    switch (feature)
    {
        case PrinterFeatureRestart:
            return send_gcode("RESTART", false);
        case PrinterFeatureFirmwareRestart:
            return send_gcode("FIRMWARE_RESTART", false);
        case PrinterFeatureHome:
            return send_gcode("G28");
        case PrinterFeatureDisableSteppers:
            return send_gcode("M18");
        case PrinterFeaturePause:
            return send_gcode("PAUSE");
        case PrinterFeatureResume:
            return send_gcode("RESUME");
        case PrinterFeatureStop:
            return send_gcode("CANCEL_PRINT");
        case PrinterFeatureEmergencyStop:
            LOG_LN("Sending estop");
            send_gcode("M112", false);
            configure_http_client(client, "/printer/emergency_stop", false, 5000, printer_config);

            try
            {
                client.GET();
            }
            catch (...)
            {
                LOG_LN("Failed to send estop");
            }

            return true;
        case PrinterFeatureExtrude:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            if (printer_config->custom_filament_move_macros)
            {
                return send_gcode("FILAMENT_EXTRUDE");
            }
            else 
            {
                return send_gcode("M83") && send_gcode("G1 E25 F300");
            }
        case PrinterFeatureRetract:
            if (printer_data.state == PrinterStatePrinting)
            {
                return false;
            }

            if (get_current_printer_config()->custom_filament_move_macros)
            {
                return send_gcode("FILAMENT_RETRACT");
            }
            else 
            {
                return send_gcode("M83") && send_gcode("G1 E-25 F300");
            }
        case PrinterFeatureCooldown:
            return send_gcode("M104 S0") && send_gcode("M140 S0");
        default:
            LOG_F(("Unsupported printer feature %d", feature));
            return false;
    }
}

bool KlipperPrinter::connect()
{
    // Pass
}


