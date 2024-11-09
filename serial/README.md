# Serial connection

This section elaborates on how the host side (linux) should be set up to handle incoming CYD-Klipper requests.

## Install (Linux)

1. Log into your host running klipper 
1. Cd into a folder where CYD-Klipper will live, for example your home folder
    - To enter your home folder, type `cd ~`
1. `git clone https://github.com/suchmememanyskill/CYD-Klipper`
1. `cd CYD-Klipper/serial`
1. `chmod a+x install.sh`
1. `./install.sh`

## Uninstall (Linux)
1. Log into your host running klipper 
1. Cd into the folder where CYD-Klipper is installed
1. `cd serial`
1. `chmod a+x uninstall.sh`
1. `./uninstall.sh`

## Configuration

The installer creates a systemd service in `~/.config/systemd/user/cyd-klipper-serial.service`. If manual configuration is needed (for example, if moonraker runs on another port or another host, or if the esp32 could not be found), you can edit this file to add environment variables.

To add an environemnt variable in a systemd unit file, inside the `[Service]` section, write `Environment="KEY=VALUE"`, where KEY is the environemnt variable's name, and VALUE is the environment variable's value.

After editing the cyd-klipper-serial.service file, run `systemctl --user daemon-reload` then `systemctl --user restart cyd-klipper-serial`

| Name | Description | Default Value |
| --- | --- | --- |
|KLIPPER_PROTOCOL|Protocol used. `http` or `https`|`http`|
|KLIPPER_HOST|The IP or hostname the Moonraker server runs on|`localhost`|
|KLIPPER_PORT|The port of the Moonraker webserver|[`80`, `7125`]|
|ESP32_SERIAL|Path to the serial device, like `/dev/ttyUSB0`|None, see below

By default, a connection to moonraker is attempted to `http://localhost:80` and `http://localhost:7125`. If no connection can be made, the script refuses to continue. The script will indefinitely attempt to re-connect. See the logs generated for more information.

By default, ESP32 devices are attempted to be located on the system. If too many are located, or none are located, the script refuses to continue. If one is found, an attempt is made to connect to the device. See the logs generated for more information.