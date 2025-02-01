#!/bin/bash

set -e

if [ "$EUID" -eq 0 ]; then
    systemctl stop cyd-klipper-serial
    systemctl disable cyd-klipper-serial

    rm /etc/systemd/system/cyd-klipper-serial.service

    systemctl daemon-reload
else
    systemctl --user stop cyd-klipper-serial
    systemctl --user disable cyd-klipper-serial

    rm ~/.config/systemd/user/cyd-klipper-serial.service

    systemctl --user daemon-reload
fi

rm -rf ./env