import subprocess, os, shutil

CYD_PORTS = ["esp32-3248S035C", "esp32-2432S028R"]
BASE_DIR = os.getcwd()

if os.path.exists("out"):
    shutil.rmtree("out")

for port in CYD_PORTS:
    port_path = os.path.join("out", port)
    os.chdir(BASE_DIR)
    os.makedirs(port_path, exist_ok=True)
    os.chdir("CYD-Klipper")
    subprocess.run(["pio", "run", "-e", port], check=True)
    os.chdir("..")
    for file in ["bootloader.bin", "partitions.bin", "firmware.bin"]:
        shutil.copy(f"./CYD-Klipper/.pio/build/{port}/{file}", f"{port_path}/{file}")

    shutil.copy(os.path.join(os.path.expanduser("~"), ".platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"), f"{port_path}/boot_app0.bin")
    os.chdir(port_path)
    subprocess.run(["python3", "-m", "esptool", "--chip", "esp32", "merge_bin", "-o", "merged_firmware.bin", "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB", "0x1000", "bootloader.bin", "0x8000", "partitions.bin", "0xe000", "boot_app0.bin", "0x10000", "firmware.bin"], check=True)
