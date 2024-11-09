#!/bin/bash

if [ "$EUID" -eq 0 ]; then
    echo "Please do not run as root"
    exit
fi

set -e

# Install dependencies
pip3 install -r requirements.txt

# Create systemd unit file
mkdir -p ~/.config/systemd/user
echo "[Unit]" > ~/.config/systemd/user/cyd-klipper-serial.service
echo "Description=CYD Klipper serial server" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "After=network.target" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "[Service]" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "ExecStart=python3 $(pwd)/serial_server.py" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "WorkingDirectory=$(pwd)" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "Restart=always" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "[Install]" >> ~/.config/systemd/user/cyd-klipper-serial.service
echo "WantedBy=multi-user.target" >> ~/.config/systemd/user/cyd-klipper-serial.service

# Start the service
systemctl --user daemon-reload
systemctl --user enable cyd-klipper-serial
systemctl --user start cyd-klipper-serial