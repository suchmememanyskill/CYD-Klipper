#include "serial_commands.h"
#include <HardwareSerial.h>
#include <Esp.h>
#include <cstring>
#include "../../conf/global_config.h"
#include "../switch_printer.h"

namespace serial_console {


HANDLER commandHandlers[] = {
    {"help", &help, 1},
    {"reset", &reset, 1},
    {"settings", &settings, 1},
    {"sets", &sets, 1},
    {"erase", &erase, 2},
    {"key", &key, 2},
    {"touch", &touch, 5},
    {"ssid", &ssid, 3},
    {"ip", &ip, 3},
    {"rotation", &rotation, 2},
    {"brightness", &brightness, 2},
    {"printer", &printer, 2}
};


// this must be here, because serial_console doesn't have a clue about sizeof(commandHandlers) at compile time
int find_command(String cmd)
{
  for(int i=0; i < sizeof(commandHandlers) / sizeof(HANDLER); ++i)
  {
    if(cmd == commandHandlers[i].name) return i; 
  }
  Serial.println("Unknown command");
  return -1;
}


void reset(String argv[])
{
    ESP.restart();
}

void help(String argv[])
{
    Serial.println("Serial console commands:");
    Serial.println("");
    Serial.println("settings             - show current settings");
    Serial.println("sets                 - show current settings as commands for copy-paste");
    Serial.println("erase [item]         - unconfigure parameter (key|touch|ssid|ip|all)");
    Serial.println("reset                - restart CYD-klipper");
    Serial.println("touch [xm xo ym yo]  - set touchscreen multipliers and offsets");
    Serial.println("ssid [name pass]     - set the network SSID and password to connect to");
    Serial.println("ip [address port]    - set Moonraker address");
    Serial.println("key [key]            - set the Moonraker API key");
    Serial.println("rotation [on|off]       - set rotate screen 180 degrees");
    Serial.println("brightness [num]     - set screen brightness");
    Serial.println("printer [num|-1]     - set active printer#; -1 for multi-printer mode off");
    Serial.println("help                 - this help");
    Serial.println("");
    Serial.println("Settings are saved immediately but come into effect after reset");
    Serial.println("Unlike GUI, serial console does not validate if settings");
    Serial.println("you enter work correctly. This is a double-edged sword.");
}
void sets(String argv[])
{

    Serial.printf("printer %d\n", global_config.multi_printer_mode?global_config.printer_index:-1);

    if(global_config.wifi_configured)
    {
        Serial.printf("ssid %s %s\n",global_config.wifi_SSID, global_config.wifi_password);
    }
    else
    {
        Serial.printf("erase ssid\n");
    }

    if(get_current_printer_config()->ip_configured)
    {
        Serial.printf("ip %s %d\n",get_current_printer_config()->klipper_host, get_current_printer_config()->klipper_port);
    }
    else
    {
        Serial.printf("erase ip\n");
    }

    if(get_current_printer_config()->auth_configured)
    {
        Serial.printf("key %s\n",get_current_printer_config()->klipper_auth);
    }
    else
    {
        Serial.printf("erase key\n");
    }

    if(global_config.screen_calibrated)
    {
        Serial.printf("touch %f %f %f %f\n",
        global_config.screen_cal_x_mult, global_config.screen_cal_x_offset, global_config.screen_cal_y_mult, global_config.screen_cal_y_offset);
    }
    else
    {
        Serial.printf("erase touch\n");
    }

    Serial.printf("rotation %s\n",global_config.rotate_screen?"on":"off");
    Serial.printf("brightness %d\n",global_config.brightness);
}

void settings(String argv[])
{

    if(get_current_printer_config()->printer_name[0] != 0)
    {
        Serial.printf("Current printer# %d name: %s",global_config.printer_index, get_current_printer_config()->printer_name);
    }
    else
    {
        Serial.printf("Current printer# %d",global_config.printer_index);
    }
    Serial.printf("  Multi-printer mode %s\n",global_config.multi_printer_mode?"enabled":"disabled");


    if(global_config.wifi_configured)
    {
        Serial.printf("SSID: %s   Password: %s\n",global_config.wifi_SSID, global_config.wifi_password);
    }
    else
    {
        Serial.printf("Wifi not configured\n");
    }

    if(get_current_printer_config()->ip_configured)
    {
        Serial.printf("Moonraker address: %s:%d\n",get_current_printer_config()->klipper_host, get_current_printer_config()->klipper_port);
    }
    else
    {
        Serial.printf("Moonraker address not configured\n");
    }

    if(get_current_printer_config()->auth_configured)
    {
        Serial.printf("Moonraker API key: %s\n",get_current_printer_config()->klipper_auth);
    }
    else
    {
        Serial.printf("Moonraker API key not configured\n");
    }

    if(global_config.screen_calibrated)
    {
        Serial.printf("Screen coefficients: x_screen = %f * x_touch + %f;  y_screen = %f * y_touch + %f\n",
        global_config.screen_cal_x_mult, global_config.screen_cal_x_offset, global_config.screen_cal_y_mult, global_config.screen_cal_y_offset);
    }
    else
    {
        Serial.printf("Screen not calibrated\n");
    }

    Serial.printf("Screen orientation: %s\n",global_config.rotate_screen?"rotated":"normal");
    Serial.printf("Screen brightness: %d\n",global_config.brightness);
}


void erase_one(const String arg)
{
    if(arg == "key")
    {
        get_current_printer_config()->auth_configured = false;
        // overwrite the key to make it unrecoverable for 3rd parties
        memset(get_current_printer_config()->klipper_auth,0,32);
        write_global_config();
    }
    else if(arg == "ip")
    {
        get_current_printer_config()->ip_configured = false;
        write_global_config();
    }
    else if(arg == "touch")
    {
        global_config.screen_calibrated = false;
        write_global_config();
    }
    else if(arg == "ssid")
    {
        global_config.wifi_configured = false;
        // overwrite the pass to make it unrecoverable for 3rd parties 
        memset(global_config.wifi_password,0,64);
        write_global_config();
    }
    else
    {
        Serial.println("Unknown key");
    }
}

void erase(String argv[])
{
    const String& arg=argv[1];
    if(arg != "all")
    {
        erase_one(arg);   
    }
    else
    {
        erase_one("key");
        erase_one("ip");
        erase_one("touch");
        erase_one("ssid");
    }
}

void key(String argv[])
{
    if (argv[1].length() != 32)
    {
      Serial.println("Key must be 32 characters");
      return;
    }

    get_current_printer_config()->auth_configured = true;
    strncpy(get_current_printer_config()->klipper_auth, argv[1].c_str(), sizeof(global_config.printer_config[0].klipper_auth));
    write_global_config();
}

void touch(String argv[])
{
    global_config.screen_cal_x_mult = argv[1].toFloat();
    global_config.screen_cal_x_offset = argv[2].toFloat();
    global_config.screen_cal_y_mult = argv[3].toFloat();
    global_config.screen_cal_y_offset = argv[4].toFloat();
    global_config.screen_calibrated = true;
    write_global_config();
}

void ssid(String argv[])
{
    strncpy(global_config.wifi_SSID, argv[1].c_str(), sizeof(global_config.wifi_SSID)-1);
    strncpy(global_config.wifi_password, argv[2].c_str(), sizeof(global_config.wifi_password)-1);
    global_config.wifi_configured = true;
    write_global_config();
}

void ip(String argv[])
{
    strncpy(get_current_printer_config()->klipper_host, argv[1].c_str(), sizeof(global_config.printer_config[0].klipper_host)-1);
    get_current_printer_config()->klipper_port =  argv[2].toInt();
    get_current_printer_config()->ip_configured = true;
    write_global_config();
}

void rotation(String argv[])
{
    if(argv[1] == "on")
    {
        global_config.rotate_screen = true;
        write_global_config();
    }
    else if (argv[1] == "off")
    {
        global_config.rotate_screen = false;
        write_global_config();
    }
    else
    {
        Serial.println("Rotation can be on or off");
    }
}

void brightness(String argv[])
{
    global_config.brightness = argv[1].toInt();
    write_global_config();
}


void printer(String argv[])
{
    int ndx = argv[1].toInt();
    if(ndx == -1)
    {
        global_config.multi_printer_mode = false;
        switch_printer(0);
    }
    else if( ndx >=0 && ndx < PRINTER_CONFIG_COUNT)
    {
        global_config.multi_printer_mode = true;
        switch_printer(ndx);
    }
    else
    {
        Serial.println("Printer index out of range");
    }

}

}