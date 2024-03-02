#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WiFi.h>

class Esp32OtaPull
{
public:
    enum ActionType { DONT_DO_UPDATE, UPDATE_BUT_NO_BOOT, UPDATE_AND_BOOT };

    // Return codes from CheckForOtaUpdate
    enum ErrorCode { UPDATE_AVAILABLE = -3, NO_UPDATE_PROFILE_FOUND = -2, NO_UPDATE_AVAILABLE = -1, UPDATE_OK = 0, HTTP_FAILED = 1, WRITE_ERROR = 2, JSON_PROBLEM = 3, OTA_UPDATE_FAIL = 4 };

private:
    void (*callback)(int offset, int totallength) = NULL;
    ActionType action = UPDATE_AND_BOOT;
    String board = ARDUINO_BOARD;
    String device = "";
    String config = "";
    String cVersion = "";
    bool downgradesAllowed = false;

    int downloadJson(const char* url, String& payload)
    {
        HTTPClient http;
        http.begin(url);

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            payload = http.getString();
        }

        // Free resources
        http.end();
        return httpResponseCode;
    }

    int doOtaUpdate(const char* url, ActionType action)
    {
        HTTPClient http;
        http.begin(url);

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200)
        {
            int totalLength = http.getSize();

            // this is required to start firmware update process
            if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                return OTA_UPDATE_FAIL;

            // create buffer for read
            uint8_t buff[1280] = { 0 };

            // get tcp stream
            WiFiClient* stream = http.getStreamPtr();

            // read all data from server
            int offset = 0;
            while (http.connected() && offset < totalLength)
            {
                size_t sizeAvail = stream->available();
                if (sizeAvail > 0)
                {
                    size_t bytesToRead = min(sizeAvail, sizeof(buff));
                    size_t bytesRead = stream->readBytes(buff, bytesToRead);
                    size_t bytesWritten = Update.write(buff, bytesRead);
                    if (bytesRead != bytesWritten)
                    {
                        // Serial.printf("Unexpected error in OTA: %d %d %d\n", bytesToRead, bytesRead, bytesWritten);
                        break;
                    }
                    offset += bytesWritten;
                    if (callback != NULL)
                        callback(offset, totalLength);
                }
            }

            if (offset == totalLength)
            {
                Update.end(true);
                delay(1000);

                // Restart ESP32 to see changes
                if (action == UPDATE_BUT_NO_BOOT)
                    return UPDATE_OK;
                ESP.restart();
            }
            return WRITE_ERROR;
        }

        http.end();
        return httpResponseCode;
    }

public:
    /// @brief Return the version string of the binary, as reported by the JSON
    /// @return The firmware version
    String getVersion()
    {
        return cVersion;
    }

    /// @brief Override the default "Device" id (MAC Address)
    /// @param device A string identifying the particular device (instance) (typically e.g., a MAC address)
    /// @return The current Esp32OtaPull object for chaining
    Esp32OtaPull &overrideDevice(const char *device)
    {
        this->device = device;
        return *this;
    }

        /// @brief Override the default "Board" value of ARDUINO_BOARD
    /// @param board A string identifying the board (class) being targeted
    /// @return The current Esp32OtaPull object for chaining
    Esp32OtaPull &overrideBoard(const char *board)
    {
        this->board = board;
        return *this;
    }

    /// @brief Specify a configuration string that must match any "Config" in JSON
    /// @param config An arbitrary string showing the current configuration
    /// @return The current Esp32OtaPull object for chaining
    Esp32OtaPull &setConfig(const char *config)
    {
        this->config = config;
        return *this;
    }

    /// @brief Specify whether downgrades (posted version is lower) are allowed
    /// @param allowDowngrades true if downgrades are allowed
    /// @return The current Esp32OtaPull object for chaining
    Esp32OtaPull &allowDowngrades(bool allowDowngrades)
    {
        this->downgradesAllowed = allowDowngrades;
        return *this;
    }

    /// @brief Specify a callback function to monitor update progress
    /// @param callback Pointer to a function that is called repeatedly during update
    /// @return The current Esp32OtaPull object for chaining
    Esp32OtaPull &setCallback(void (*callback)(int offset, int totallength))
    {
        this->callback = callback;
        return *this;
    }

    /// @brief The main entry point for OTA Update
    /// @param JSON_URL The URL for the JSON filter file
    /// @param currentVersion The version # of the current (i.e. to be replaced) sketch
    /// @param action The action to be performed.  May be any of DONT_DO_UPDATE, UPDATE_BUT_NO_BOOT, UPDATE_AND_BOOT (default)
    /// @return ErrorCode or HTTP failure code (see enum above)
    int checkForOtaUpdate(const char* jsonUrl, const char *currentVersion, ActionType action = UPDATE_AND_BOOT)
    {
        currentVersion = currentVersion == NULL ? "" : currentVersion;

        // Downloading OTA Json...
        String payload;
        int httpResponseCode = downloadJson(jsonUrl, payload);
        if (httpResponseCode != 200)
            return httpResponseCode > 0 ? httpResponseCode : HTTP_FAILED;

        // Deserialize the JSON file downloaded from user's site
        JsonDocument doc;
        DeserializationError deserialization = deserializeJson(doc, payload.c_str());
        if (deserialization != DeserializationError::Ok)
            return JSON_PROBLEM;

        String deviceName = device.isEmpty() ? WiFi.macAddress() : device;
        String boardName = board.isEmpty() ? ARDUINO_BOARD : board;
        String configName = config.isEmpty() ? "" : config;
        bool foundProfile = false;

        // Step through the configurations looking for a match
        for (auto config : doc["Configurations"].as<JsonArray>())
        {
            String cBoard = config["Board"].isNull() ? "" : (const char *)config["Board"];
            String cDevice = config["Device"].isNull() ? "" : (const char *)config["Device"];
            cVersion = config["Version"].isNull() ? "" : (const char *)config["Version"];
            String cConfig = config["Config"].isNull() ? "" : (const char *)config["Config"];
            //Serial.printf("Checking %s %s %s %s\n", cBoard.c_str(), cDevice.c_str(), cVersion.c_str(), cConfig.c_str());
            //Serial.printf("Against %s %s %s %s\n", boardName.c_str(), deviceName.c_str(), currentVersion, configName.c_str());
            if ((cBoard.isEmpty() || cBoard == boardName) &&
                (cDevice.isEmpty() || cDevice == deviceName) &&
                (cConfig.isEmpty() || cConfig == configName))
            {
                if (cVersion.isEmpty() || cVersion > String(currentVersion) ||
                    (downgradesAllowed && cVersion != String(currentVersion)))
                    return action == DONT_DO_UPDATE ? UPDATE_AVAILABLE : doOtaUpdate(config["URL"], action);
                foundProfile = true;
            }
        }
        return foundProfile ? NO_UPDATE_AVAILABLE : NO_UPDATE_PROFILE_FOUND;
    }
};
