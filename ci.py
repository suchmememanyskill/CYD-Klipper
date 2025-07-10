import subprocess, os, shutil, json

BASE_DIR = os.getcwd()

def get_manifest(base_path : str, device_name : str, is_s3 : bool):
    return {
        "name": f"to {device_name}",
        "funding_url": "https://ko-fi.com/suchmememanyskill",
        "new_install_prompt_erase": True,
        "builds": [
            {
                "chipFamily": "ESP32-S3" if is_s3 else "ESP32",
                "parts": [
                    {
                        "path": f"{base_path}/bootloader.bin",
                        "offset": 0 if is_s3 else 0x1000 
                    },
                    {
                        "path": f"{base_path}/partitions.bin",
                        "offset": 0x8000
                    },
                    {
                        "path": f"{base_path}/boot_app0.bin",
                        "offset": 0xe000
                    },
                    {
                        "path": f"{base_path}/firmware.bin",
                        "offset": 0x10000
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
site_sections : dict[str, dict] = {"CYD": []}

def add_configuration(board : str):
    configurations.append({
        "Board": board,
        "Version": repo_version,
        "URL": f"https://suchmememanyskill.github.io/CYD-Klipper/out/{board}/firmware.bin"
    })

def add_site_section(port : str, data : dict[str, bool|str]):
    brand = data["brand"] if "brand" in data else "CYD"
    
    if brand not in site_sections:
        site_sections[brand] = []

    site_sections[brand].append({
        "name": data["name"],
        "port": port,
        "default": "default" in data and data["default"],
    })

if os.path.exists("out"):
    shutil.rmtree("out")

if not os.path.exists("_site"):
    os.makedirs("_site")

with open("./ci.json", "r") as fp:
    ci_data : dict[str, dict[str, bool|str]] = json.load(fp)

for port, data in ci_data.items():
    if "skip" in data and data["skip"]:
        print(f"Skipping {port}...")
        continue

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
    if (bool(data["s3"])):
        subprocess.run(["esptool", "--chip", "esp32s3", "merge_bin", "-o", "merged_firmware.bin", "--flash_mode", "dio", "--flash_freq", "80m", "--flash_size", "16MB", "0x0000", "bootloader.bin", "0x8000", "partitions.bin", "0xe000", "boot_app0.bin", "0x10000", "firmware.bin"], check=True)
    else:    
        subprocess.run(["esptool", "--chip", "esp32", "merge_bin", "-o", "merged_firmware.bin", "--flash_mode", "dio", "--flash_freq", "40m", "--flash_size", "4MB", "0x1000", "bootloader.bin", "0x8000", "partitions.bin", "0xe000", "boot_app0.bin", "0x10000", "firmware.bin"], check=True)

    os.chdir(BASE_DIR)

    with open(f"./_site/{port}.json", "w") as f:
        json.dump(get_manifest(port_path, port, bool(data["s3"])), f)

    add_configuration(port)

    if "site" in data and data["site"]:
        add_site_section(port, data)

os.chdir(BASE_DIR)
out_dir = "./_site/out"
if os.path.exists(out_dir):
    shutil.rmtree(out_dir)
shutil.copytree("./out", out_dir)

with open("./_site/OTA.json", "w") as f:
    json.dump({"Configurations": configurations}, f)

with open("./_site/index.html", "w") as fp:
    with open("./template.html", "r") as template_fp:
        template = template_fp.read()

    insert_html = ""
    
    for brand, sections in site_sections.items():
        option_htmls = [f"<option {'selected' if x['default'] else ''} value=\"{x['port']}\">{x['name']}</option>" for x in sections]
        section_html = f"<optgroup label=\"{brand}\">{''.join(option_htmls)}</optgroup>"
        insert_html += section_html

    fp.write(template.replace("{{%PORTS%}}", insert_html))