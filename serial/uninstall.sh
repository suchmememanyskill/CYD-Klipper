#!/bin/bash

if [ "$EUID" -eq 0 ]; then
    echo "Please do not run as root"
    exit
fi

systemctl --user stop cyd-klipper-serial
systemctl --user disable cyd-klipper-serial

rm ~/.config/systemd/user/cyd-klipper-serial.service

systemctl --user daemon-reload