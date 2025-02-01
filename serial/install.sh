#!/bin/bash

if [ "$EUID" -eq 0 ]; then
    read -r -p "Are you sure you want to run this service as root? [y/N] " response
    response=${response,,}    # tolower
    if ! [[ "$response" =~ ^(yes|y)$ ]]; then
        exit
    fi
    SERVICE_PATH="/etc/systemd/system/cyd-klipper-serial.service"
else
    echo "Are you sure you want to run this service as the current user?"
    read -r -p "Make sure this user is logged in at boot! [y/N] " response
    response=${response,,}    # tolower
    if ! [[ "$response" =~ ^(yes|y)$ ]]; then
        exit
    fi
    mkdir -p ~/.config/systemd/user
    SERVICE_PATH="$HOME/.config/systemd/user/cyd-klipper-serial.service"
fi

set -e

chmod a+x ./run.sh

# Install dependencies
python3 -m venv ./env
source ./env/bin/activate
pip3 install -r requirements.txt

# Create systemd unit file

echo "[Unit]" > $SERVICE_PATH
echo "Description=CYD Klipper serial server" >> $SERVICE_PATH
echo "After=network.target" >> $SERVICE_PATH
echo "" >> $SERVICE_PATH
echo "[Service]" >> $SERVICE_PATH
echo "ExecStart=$(pwd)/run.sh" >> $SERVICE_PATH
echo "WorkingDirectory=$(pwd)" >> $SERVICE_PATH
echo "Restart=always" >> $SERVICE_PATH
echo "" >> $SERVICE_PATH
echo "[Install]" >> $SERVICE_PATH
echo "WantedBy=multi-user.target" >> $SERVICE_PATH

# Start the service
if [ "$EUID" -eq 0 ]; then
    systemctl daemon-reload
    systemctl enable cyd-klipper-serial
    systemctl start cyd-klipper-serial
else
    systemctl --user daemon-reload
    systemctl --user enable cyd-klipper-serial
    systemctl --user start cyd-klipper-serial
fi