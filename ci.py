import subprocess, os, shutil, json

CYD_PORTS = ["esp32-3248S035C", "esp32-2432S028R", "esp32-2432S032C-SD"]
BASE_DIR = os.getcwd()

def get_manifest(base_path : str, device_name : str):
    return {
        "name": f"to {device_name}",
        "new_install_prompt_erase": True,
        "builds": [
            {
                "chipFamily": "ESP32",
                "parts": [
                    {
                        "path": f"{base_path}/bootloader.bin",
                        "offset": 4096
                    },
                    {
                        "path": f"{base_path}/partitions.bin",
                        "offset": 32768
                    },
                    {
                        "path": f"{base_path}/boot_app0.bin",
                        "offset": 57344
                    },
                    {
                        "path": f"{base_path}/firmware.bin",
                        "offset": 65536
                    }
                ]
            }
        ]
    }

def extract_commit() -> str:
    git_describe_output = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True, check=True).stdout.strip()
    return git_describe_output.split("-")[0]

repo_version = extract_commit()
configurations = []

def add_configuration(board : str):
    configurations.append({
        "Board": board,
        "Version": repo_version,
        "URL": f"https://suchmememanyskill.github.io/CYD-Klipper/out/{board}/firmware.bin"
    })

if os.path.exists("out"):
    shutil.rmtree("out")

for port in CYD_PORTS:
    port_path = f"out/{port}"
    os.chdir(BASE_DIR)
    os.makedirs(port_path, exist_ok=True)
    os.chdir("CYD-Klipper")
    subprocess.run(["pio", "run", "-e", port], check=True)
    os.chdir(BASE_DIR)
    for file in ["bootloader.bin", "partitions.bin", "firmware.bin"]:
        shutil.copy(f"./CYD-Klipper/.pio/build/{port}/{file}", f"{port_path}/{file}")

    shutil.copy(os.path.join(os.path.expanduser("~"), ".platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"), f"{port_path}/boot_app0.bin")
    os.chdir(port_path)
    subprocess.run(["python3", "-m", "esptool", "--chip", "esp32", "merge_bin", "-o", "merged_firmware.bin", "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB", "0x1000", "bootloader.bin", "0x8000", "partitions.bin", "0xe000", "boot_app0.bin", "0x10000", "firmware.bin"], check=True)

    os.chdir(BASE_DIR)

    with open(f"./_site/{port}.json", "w") as f:
        json.dump(get_manifest(port_path, port), f)

    add_configuration(port)

os.chdir(BASE_DIR)
shutil.copytree("./out", "./_site/out")

with open("./_site/OTA.json", "w") as f:
    json.dump({"Configurations": configurations}, f)
