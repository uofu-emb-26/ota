# ESP32 OTA Usage Guide

## Overview
The ESP32 acts as a WiFi-to-UART middleman. It fetches a firmware binary
from a host PC over WiFi and stores it in flash, then forwards it to an
STM32 over UART2. It also polls the host PC periodically for new versions
and updates automatically when one is available.

## Requirements
- PlatformIO extension in VS Code
- Python 3 installed on host PC
- ESP32 and host PC on the same WiFi network
- Make sure 2.4GHz is enabled when using hotspot

## Setup

### 1. Configure WiFi credentials
In `src/main.cpp`, update with your network credentials:
```cpp
const char* ssid = "your_network";
const char* password = "your_password";
```

### 2. Configure the server URL
In `fetchBinary()` and `checkForUpdate()`, update the IP and filenames:
```cpp
http.begin("http://<your-PC-IP>:8080/yourfile.bin");  // in fetchBinary()
http.begin("http://<your-PC-IP>:8080/version.txt");   // in checkForUpdate()
```

To get IP:
```bash
# Mac/Linux
ifconfig getifaddr en0

# Windows
ipconfigip
```


### 3. Build and upload
- Through PlatformIO menu, select Build
- Once that is done and device is connected, select Upload and Monitor

### 4. Find the ESP32 IP address
Open the Serial Monitor (plug icon, 115200 baud). You should see:
```
Connected! IP: 192.168.x.x
```

### 5. Start the HTTP server on your PC
Navigate to the folder containing your `.bin` and `version.txt` files:
```bash
# Mac/Linux
python3 -m http.server 8080

# Windows
python -m http.server 8080
```

## Automatic Update Polling
The ESP32 checks for updates every 30 seconds by fetching `version.txt`
from the host PC and comparing it to the current version. If the server
version is higher, it automatically fetches and stores the new binary.

To release a new version:
1. Replace the `.bin` file on your PC with the new binary
2. Increment the number in `version.txt` (e.g. `1` -> `2`)

To change the polling interval, update `CHECK_INTERVAL` in `main.cpp`:
```cpp
const unsigned long CHECK_INTERVAL = 30000; // milliseconds
```

## HTTP Endpoints
| Endpoint | Description |
|---|---|
| `/fetch` | Manually fetch binary from host PC and store in flash |
| `/verify` | Read stored binary, print size and first 16 bytes |
| `/send_binary` | Send stored binary over UART2 to STM32 |
| `/send?msg=hello` | Send a text message over UART2 |
| `/flash` | Blink onboard LED 5 times (connectivity test) |

## Hardware Wiring (ESP32 to STM32)
| ESP32 | STM32 |
|---|---|
| GPIO17 (TX) | RX pin |
| GPIO16 (RX) | TX pin |
| GND | GND |

---




# ESP32 Upload Fix for PlatformIO in VS Code (WSL Only)

This guide is **only for users running VS Code with WSL**.
If you are not using WSL, do not follow this guide.

## Problem

Build works, but upload fails with a serial permission error like:

- `Could not open /dev/ttyUSB0`
- `Permission denied: '/dev/ttyUSB0'`
- Hint about adding user to `dialout` or `uucp`

In WSL, this usually means your Linux user is not in the group that owns the USB serial device.

## Why This Happens

`/dev/ttyUSB0` is commonly owned by group `dialout`.
If your user is not in that group, PlatformIO/esptool cannot open the port for upload.

## One-Time Fix (WSL)

Run this command in your WSL terminal from any folder:

```bash
sudo usermod -aG dialout $USER
```

Enter your sudo password when prompted.

## Important: Reload WSL Session (Required)

Group membership changes do not apply to already-running shells.
Do the following in order:

1. Close all VS Code WSL windows.
2. Close all WSL terminals.
3. In **Windows PowerShell**, run:

```powershell
wsl --shutdown
```

4. Reopen VS Code in WSL and open this project again.

## Verify Group Membership After Reopen

In the new WSL terminal, run this exact command:

```bash
groups
```

You should see `dialout` in the output.

## Upload Again

Run:

```bash
platformio run --target upload
```

## If It Still Fails

Run these commands and inspect/share their output:

```bash
ls -l /dev/ttyUSB0
fuser /dev/ttyUSB0
platformio run --target upload -v
```

- If another process is using the port, stop that process and retry.
- Ensure the board is still exposed to WSL as `/dev/ttyUSB0`.


