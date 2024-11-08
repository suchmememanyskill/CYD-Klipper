import serial
import requests
import time

# Define serial port and parameters
SERIAL_PORT = 'COM6'  # Change this to your serial port
BAUD_RATE = 115200
HOSTNAME = 'http://localhost'  # Change to your hostname
PORT = 7125  # Change to your port if necessary

# --------- #

ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

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
            if line.startswith("HTTP_REQUEST"):
                print(f">>> {line}")
                # Parse the parameters
                try:
                    parts = line.split(' ', 3)
                    timeout_ms, request_type, url_path = int(parts[1]), parts[2], parts[3]
                    
                    ignore_timeout = timeout_ms <= 0

                    if ignore_timeout:
                        timeout_ms = 1000;

                    # Construct the full URL
                    full_url = f"{HOSTNAME}:{PORT}{url_path}"
                    
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
    try:
        main()
    except KeyboardInterrupt:
        print("\nExiting script.")
        ser.close()