import serial
import requests
import os
import serial.tools.list_ports
import time

SERIAL_PORT = 'COM6'
BAUD_RATE = 115200
PROTOCOL = "http"
HOSTNAME = 'localhost'
PORT = 80 

def test_request(protocol : str, hostname : str, port : int) -> bool:
    try:
        print(f"Sending test request to {hostname}:{port}")
        response = requests.get(f"{protocol}://{hostname}:{port}/printer/info")
        return response.status_code == 200
    except requests.exceptions.RequestException:
        return False

def find_klipper_host() -> bool:
    global PROTOCOL, HOSTNAME, PORT

    protocol = PROTOCOL
    host = HOSTNAME
    port = PORT

    if "KLIPPER_PROTOCOL" in os.environ:
        protocol = os.environ["KLIPPER_PROTOCOL"]

    if "KLIPPER_HOST" in os.environ:
        host = os.environ["KLIPPER_HOST"]

    if "KLIPPER_PORT" in os.environ:
        port = int(os.environ["KLIPPER_PORT"])
    
    if test_request(protocol, host, port):
        HOSTNAME = host
        PORT = port
        return True

    port = 80

    if test_request(protocol, host, port):
        HOSTNAME = host
        PORT = port
        return True

    port = 7125

    if test_request(protocol, host, port):
        HOSTNAME = host
        PORT = port
        return True

    print("Could not find Klipper host. Please specify the hostname and port using the KLIPPER_HOST and KLIPPER_PORT environment variables.")
    return False

def find_esp32() -> bool:
    global SERIAL_PORT

    if "ESP32_SERIAL" in os.environ:
        SERIAL_PORT = os.environ["ESP32_SERIAL"]

        if os.path.exists(SERIAL_PORT):
            return True
        else:
            print(f"Specified serial port {SERIAL_PORT} does not exist.")
    
    possible_devices = []

    for port in serial.tools.list_ports.comports():
        if port.vid == 0x10C4 and port.pid == 0xEA60:
            possible_devices.append(port)
        elif port.vid == 0x1A86 and port.pid == 0x7523:
            possible_devices.append(port)
    
    if len(possible_devices) == 1:
        SERIAL_PORT = possible_devices[0].device
        return True
    elif len(possible_devices) > 1:
        print("Multiple ESP32 devices found. Please specify the serial port using the ESP32_SERIAL environment variable.")
        return False
    else:
        print("No ESP32 devices found. Please specify the serial port using the ESP32_SERIAL environment variable.")
        return False
            
# --------- #

ser : serial.Serial = None

def truncuate(text : str, length : int = 50):
    length = length - 3
    if len(text) > length:
        return text[:length] + "..."
    return text

def write(text : str, write : bool):
    if write:
        ser.write((text + "\n").encode('utf-8'))
        print(f"<<< {truncuate(text)}")
    else:
        print(f"(Ignored) <<< {truncuate(text)}")

def main():
    while True:
        # Read a line from the serial port
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith("HTTP_REQUEST") or line.startswith("HTTP_BINARY"):
                print(f">>> {line}")
                # Parse the parameters
                try:
                    parts = line.split(' ', 3)
                    timeout_ms, request_type, url_path = int(parts[1]), parts[2], parts[3]
                    
                    ignore_timeout = timeout_ms <= 0
                    binary = line.startswith("HTTP_BINARY")

                    if ignore_timeout:
                        timeout_ms = 1000;

                    # Construct the full URL
                    full_url = f"{PROTOCOL}://{HOSTNAME}:{PORT}{url_path}"
                    
                    # Make the HTTP request based on the type
                    response = None
                    if request_type.upper() == "GET":
                        response = requests.get(full_url, timeout=timeout_ms / 1000)
                    elif request_type.upper() == "POST":
                        response = requests.post(full_url, timeout=timeout_ms / 1000)
                    else:
                        write("400 Unsupported request type", not ignore_timeout)
                        continue

                    # Send response back over serial
                    if response != None:
                        if binary:
                            if response.status_code != 200:
                                write("00000000", not ignore_timeout)
                            else:
                                length = len(response.content)
                                ser.write(f"{length:>08}".encode('utf-8'))
                                ser.write(response.content)
                                print(f"<<< (Binary data of {length} bytes)")
                        else:
                            status_code = response.status_code
                            body = response.text.replace('\n', ' ')  # Trim and sanitize body for serial
                            message = f"{status_code} {body}"
                            write(message, not ignore_timeout)
                except (IndexError, ValueError) as e:
                    write(f"400 Malformed request {str(e)}", not ignore_timeout)
                except requests.exceptions.ReadTimeout as e:
                    print("Request timed out.")
                    pass
                except requests.exceptions.RequestException as e:
                    write("500 Request failed", not ignore_timeout)
            else:
                print(f"[LOG] {line}")

if __name__ == "__main__":
    while True:
        try:
            if not find_klipper_host():
                time.sleep(5);
                continue

            if not find_esp32():
                time.sleep(5);
                continue

            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            main()
        except KeyboardInterrupt:
            print("\nExiting script.")
            ser.close()
            break;
        except Exception as e:
            print(f"An error occurred: {str(e)}")
            ser.close()
            print("Retrying in 5 seconds...")
            time.sleep(5)
            