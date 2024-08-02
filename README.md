![GitHub release (with filter)](https://img.shields.io/github/v/release/suchmememanyskill/CYD-Klipper)
[![Donations](https://img.shields.io/badge/Support%20on-Ko--Fi-red)](https://ko-fi.com/suchmememanyskill)

# CYD-Klipper
An implementation of a wireless Klipper status display on an ESP32 + screen. Uses Moonraker to fetch data.

A simple and cheap solution to use a dedicated screen with Klipper, a 3d printing Firmware.

![showcase_image](readme/PXL_20231113_171629383.jpg)

### Required hardware

A ESP32-2432S028R is required to run this project. You can find out where to buy these on the ["ESP32 Cheap Yellow Display"](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display#where-to-buy) repository.

### Features
- View printer status
- View print progress and print statistics
- Start a print
- (When the printer is idle) move the printer
- (During a print) set fan speed, flow rate, speed and z offset
- Manage temperature
- Extrude/Retract filament
- Execute predefined gcode macros
- Toggle Moonraker power devices
- OTA updates
- Serial console over USB (115200 8n1, echo off, LF/LF)

### Install

[There is a web-based installer available. This is only supported on Chrome, Edge, Arc or Opera, and only on Desktop.](https://suchmememanyskill.github.io/CYD-Klipper/)

On initial install, all data should be wiped. On updates, data should be able to be kept without issues.

When there is an update available, a button in the settings will appear that can be pressed to update. If automatic updates are preferred, there is a toggle in the settings to automatically update. This will right after connecting to wifi update the screen.

### Donate

If you found this project helpful, please consider a donation [to my Ko-Fi](https://ko-fi.com/suchmememanyskill). It would help out a lot in the development of this project, due to the need to buy the screens. 

Thank you!

### Screenshots
(Quite literally shots of the screen. I'm sorry)

-|- 
:-:|:-:
![1](readme/1.jpg)|![2](readme/2.jpg)
![3](readme/3.jpg)|![4](readme/4.jpg)
![5](readme/5.jpg)|![6](readme/6.jpg)
![7](readme/7.jpg)|![8](readme/8.jpg)
![9](readme/9.jpg)|![10](readme/10.jpg)


### Credits
- [xtouch](https://github.com/xperiments-in/xtouch)
- [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
- [OperatorB](https://github.com/OperatorB) for the ESP32-3248S035C display driver
- [esp32-smartdisplay](https://github.com/rzeldent/esp32-smartdisplay)
