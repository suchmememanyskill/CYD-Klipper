import subprocess, json

latest_tag = subprocess.check_output(["git", "describe", "--tags", "--abbrev=0"])

data_without_wipe = {
    "name": "CYD-Klipper-Display",
    "new_install_prompt_erase": False,
    "version": latest_tag.decode("utf-8").strip(),
    "builds": [
        {
            "chipFamily": "ESP32",
            "parts": [
                {
                    "path": "output/bootloader.bin",
                    "offset": 4096
                },
                {
                    "path": "output/partitions.bin",
                    "offset": 32768
                },
                {
                    "path": "output/boot_app0.bin",
                    "offset": 57344
                },
                {
                    "path": "output/firmware.bin",
                    "offset": 65536
                }
            ]
        }
    ]
}

data_with_wipe = {
    "name": "CYD-Klipper-Display",
    "new_install_prompt_erase": False,
    "version": latest_tag.decode("utf-8").strip(),
    "builds": [
        {
            "path": "output/merged-firmware.bin",
            "offset": 0
        }
    ]
}

with open("manifest.json", "w") as f:
    json.dump(data_without_wipe, f, indent=4)

with open("manifest_wipe.json", "w") as f:
    json.dump(data_with_wipe, f, indent=4)