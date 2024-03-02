#include "data_setup.h"
#include "lvgl.h"
#include "../conf/global_config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "macros_query.h"
#include <UrlEncode.h>

const char *printerStateMessages[] = {
    "Error",
    "Idle",
    "Printing"};

Printer printer = {0};
int klipperRequestConsecutiveFailCount = 0;
char filenameBuff[512] = {0};
SemaphoreHandle_t freezeRenderThreadSemaphore, freezeRequestThreadSemaphore;
const long dataUpdateInterval = 780;

void SemaphoreInit()
{
    freezeRenderThreadSemaphore = xSemaphoreCreateMutex();
    freezeRequestThreadSemaphore = xSemaphoreCreateMutex();
    xSemaphoreGive(freezeRenderThreadSemaphore);
    xSemaphoreGive(freezeRequestThreadSemaphore);
}

void FreezeRequestThread()
{
    xSemaphoreTake(freezeRequestThreadSemaphore, portMAX_DELAY);
}

void UnfreezeRequestThread()
{
    xSemaphoreGive(freezeRequestThreadSemaphore);
}

void FreezeRenderThread()
{
    xSemaphoreTake(freezeRenderThreadSemaphore, portMAX_DELAY);
}

void UnfreezeRenderThread()
{
    xSemaphoreGive(freezeRenderThreadSemaphore);
}

void SendGcode(bool wait, const char *gcode)
{
    Serial.printf("Sending gcode: %s\n", gcode);
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/printer/gcode/script?script=%s", globalConfig.klipperHost, globalConfig.klipperPort, urlEncode(gcode).c_str());
    HTTPClient client;
    client.begin(buff);

    if (globalConfig.authConfigured)
        client.addHeader("X-Api-Key", globalConfig.klipperAuth);

    if (!wait)
    {
        client.setTimeout(1000);
    }

    try
    {
        client.GET();
    }
    catch (...)
    {
        Serial.println("Failed to send gcode");
    }
}

int GetSlicerTimeEstimateS()
{
    if (printer.state == PRINTER_STATE_IDLE)
        return 0;

    delay(10);

    char buff[256] = {};
    sprintf(buff, "http://%s:%d/server/files/metadata?filename=%s", globalConfig.klipperHost, globalConfig.klipperPort, urlEncode(printer.printFilename).c_str());
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(buff);

    if (globalConfig.authConfigured)
        client.addHeader("X-Api-Key", globalConfig.klipperAuth);

    int httpCode = client.GET();

    if (httpCode != 200)
        return 0;

    JsonDocument doc;
    deserializeJson(doc, client.getStream());
    int timeEstimateS = doc["result"]["estimated_time"];
    Serial.printf("Got slicer time estimate: %ds\n", timeEstimateS);
    return timeEstimateS;
}

void MovePrinter(const char *axis, float amount, bool relative)
{
    if (!printer.homedAxis || printer.state == PRINTER_STATE_PRINTING)
        return;

    char gcode[64];
    const char *extra = (amount > 0) ? "+" : "";

    bool absoluteCoords = printer.absoluteCoords;

    if (absoluteCoords && relative)
    {
        SendGcode(true, "G91");
    }
    else if (!absoluteCoords && !relative)
    {
        SendGcode(true, "G90");
    }

    sprintf(gcode, "G1 %s%s%.3f F6000", axis, extra, amount);
    SendGcode(true, gcode);

    if (absoluteCoords && relative)
    {
        SendGcode(true, "G90");
    }
    else if (!absoluteCoords && !relative)
    {
        SendGcode(true, "G91");
    }
}

int lastSlicerTimeQuery = -15000;

void FetchPrinterData()
{
    FreezeRequestThread();
    char buff[256] = {};
    sprintf(buff, "http://%s:%d/printer/objects/query?extruder&heater_bed&toolhead&gcode_move&virtual_sdcard&print_stats&webhooks&fan", globalConfig.klipperHost, globalConfig.klipperPort);
    HTTPClient client;
    client.useHTTP10(true);
    client.begin(buff);

    if (globalConfig.authConfigured)
        client.addHeader("X-Api-Key", globalConfig.klipperAuth);

    int httpCode = client.GET();
    delay(10);
    if (httpCode == 200)
    {
        klipperRequestConsecutiveFailCount = 0;
        JsonDocument doc;
        deserializeJson(doc, client.getStream());
        auto status = doc["result"]["status"];
        bool emitStateUpdate = false;
        int printerState = printer.state;
        delay(10);
        UnfreezeRequestThread();
        FreezeRenderThread();

        if (status.containsKey("webhooks"))
        {
            const char *state = status["webhooks"]["state"];
            const char *message = status["webhooks"]["state_message"];

            if (strcmp(state, "ready") == 0 && printer.state == PRINTER_STATE_ERROR)
            {
                printerState = PRINTER_STATE_IDLE;
            }
            else if (strcmp(state, "shutdown") == 0 && printer.state != PRINTER_STATE_ERROR)
            {
                printerState = PRINTER_STATE_ERROR;
            }

            if (printer.stateMessage == NULL || strcmp(printer.stateMessage, message))
            {
                if (printer.stateMessage != NULL)
                {
                    free(printer.stateMessage);
                }

                printer.stateMessage = (char *)malloc(strlen(message) + 1);
                strcpy(printer.stateMessage, message);
                emitStateUpdate = true;
            }
        }

        if (printerState != PRINTER_STATE_ERROR)
        {
            if (status.containsKey("extruder"))
            {
                printer.extruderTemp = status["extruder"]["temperature"];
                printer.extruderTargetTemp = status["extruder"]["target"];
                bool canExtrude = status["extruder"]["can_extrude"];
                printer.pressureAdvance = status["extruder"]["pressure_advance"];
                printer.smoothTime = status["extruder"]["smooth_time"];
                printer.canExtrude = canExtrude == true;
            }

            if (status.containsKey("heater_bed"))
            {
                printer.bedTemp = status["heater_bed"]["temperature"];
                printer.bedTargetTemp = status["heater_bed"]["target"];
            }

            if (status.containsKey("toolhead"))
            {
                const char *homedAxis = status["toolhead"]["homed_axes"];
                printer.homedAxis = strcmp(homedAxis, "xyz") == 0;
            }

            if (status.containsKey("gcode_move"))
            {
                printer.position[0] = status["gcode_move"]["gcode_position"][0];
                printer.position[1] = status["gcode_move"]["gcode_position"][1];
                printer.position[2] = status["gcode_move"]["gcode_position"][2];
                printer.gcodeOffset[0] = status["gcode_move"]["homing_origin"][0];
                printer.gcodeOffset[1] = status["gcode_move"]["homing_origin"][1];
                printer.gcodeOffset[2] = status["gcode_move"]["homing_origin"][2];
                bool absoluteCoords = status["gcode_move"]["absolute_coordinates"];
                printer.absoluteCoords = absoluteCoords == true;
                printer.speedMult = status["gcode_move"]["speed_factor"];
                printer.extrudeMult = status["gcode_move"]["extrude_factor"];
                printer.feedrateMmPerS = status["gcode_move"]["speed"];
                printer.feedrateMmPerS /= 60; // convert mm/m to mm/s
            }

            if (status.containsKey("fan"))
            {
                printer.fanSpeed = status["fan"]["speed"];
            }

            if (status.containsKey("virtual_sdcard"))
            {
                printer.printProgress = status["virtual_sdcard"]["progress"];
            }

            if (status.containsKey("print_stats"))
            {
                const char *filename = status["print_stats"]["filename"];
                strcpy(filenameBuff, filename);
                printer.printFilename = filenameBuff;
                printer.elapsedTime = status["print_stats"]["print_duration"];
                printer.filamentUsedMm = status["print_stats"]["filament_used"];
                printer.totalLayers = status["print_stats"]["info"]["total_layer"];
                printer.currentLayer = status["print_stats"]["info"]["current_layer"];

                const char *state = status["print_stats"]["state"];

                if (state == nullptr)
                {
                    printerState = PRINTER_STATE_ERROR;
                }
                else if (strcmp(state, "printing") == 0)
                {
                    printerState = PRINTER_STATE_PRINTING;
                }
                else if (strcmp(state, "paused") == 0)
                {
                    printerState = PRINTER_STATE_PAUSED;
                }
                else if (strcmp(state, "complete") == 0 || strcmp(state, "cancelled") == 0 || strcmp(state, "standby") == 0)
                {
                    printerState = PRINTER_STATE_IDLE;
                }
            }

            // TODO: make a call to /server/files/metadata to get more accurate time estimates
            // https://moonraker.readthedocs.io/en/latest/web_api/#server-administration

            if (printer.state == PRINTER_STATE_PRINTING && printer.printProgress > 0)
            {
                float remainingTimePercentage = (printer.elapsedTime / printer.printProgress) - printer.elapsedTime;
                float remainingTimelicer = 0;

                if (printer.slicerEstimatedPrintTime > 0)
                {
                    remainingTimelicer = printer.slicerEstimatedPrintTime - printer.elapsedTime;
                }

                if (remainingTimelicer <= 0 || globalConfig.remainingTimeCalcMode == REMAINING_TIME_CALC_PERCENTAGE)
                {
                    printer.remainingTime = remainingTimePercentage;
                }
                else if (globalConfig.remainingTimeCalcMode == REMAINING_TIME_CALC_INTERPOLATED)
                {
                    printer.remainingTime = remainingTimePercentage * printer.printProgress + remainingTimelicer * (1 - printer.printProgress);
                }
                else if (globalConfig.remainingTimeCalcMode == REMAINING_TIME_CALC_SLICER)
                {
                    printer.remainingTime = remainingTimelicer;
                }
            }

            if (printer.remainingTime < 0)
            {
                printer.remainingTime = 0;
            }

            if (printer.state == PRINTER_STATE_IDLE)
            {
                printer.slicerEstimatedPrintTime = 0;
            }

            lv_msg_send(DATA_PRINTER_DATA, &printer);
        }

        if (printer.state != printerState || emitStateUpdate)
        {
            printer.state = printerState;
            lv_msg_send(DATA_PRINTER_STATE, &printer);
        }

        if (printer.state == PRINTER_STATE_PRINTING && millis() - lastSlicerTimeQuery > 30000 && printer.slicerEstimatedPrintTime <= 0)
        {
            delay(10);
            lastSlicerTimeQuery = millis();
            printer.slicerEstimatedPrintTime = GetSlicerTimeEstimateS();
        }

        UnfreezeRenderThread();
    }
    else
    {
        klipperRequestConsecutiveFailCount++;
        Serial.printf("Failed to fetch printer data: %d\n", httpCode);
        UnfreezeRequestThread();
    }
}

void DataLoop()
{
    // Causes other threads that are trying to lock the thread to actually lock it
    UnfreezeRenderThread();
    delay(1);
    FreezeRenderThread();
}

void DataLoopBackground(void *param)
{
    esp_task_wdt_init(10, true);
    while (true)
    {
        delay(dataUpdateInterval);
        FetchPrinterData();
    }
}

TaskHandle_t backgroundLoop;

void DataSetup()
{
    SemaphoreInit();
    printer.printFilename = filenameBuff;
    FetchPrinterData();
    MacrosQuerySetup();
    FreezeRenderThread();
    xTaskCreatePinnedToCore(DataLoopBackground, "data_loop_background", 5000, NULL, 0, &backgroundLoop, 0);
}