# ESP32 OTA Usage Guide

## Overview
The ESP32 acts as a WiFi-to-UART middleman. It fetches a firmware binary
from a host PC over WiFi and stores it in flash, then forwards it to an
STM32 over UART2. Updates are triggered via MQTT messages sent through
HiveMQ Cloud, no local server required.

## Requirements
- PlatformIO extension in VS Code
- ESP32 on any WiFi network with internet access
- HiveMQ Cloud account (free tier)
- Make sure 2.4GHz is enabled when using hotspot

## Setup

### 1. Create your local config file
Copy the template and set your local secrets:

```bash
cp src/config.example.h src/config.h
```

Then edit `src/config.h` with your own WiFi and MQTT values.

`src/config.h` is intentionally gitignored, so your local secrets are not tracked.

### 2. Configure MQTT credentials

In [`src/main.cpp` lines 24–27](src/main.cpp#L24-L27), update with your HiveMQ cluster details:

```cpp
#define MQTT_BROKER "your-cluster.hivemq.cloud"
#define MQTT_USER   "your_username"
#define MQTT_PASS   "your_password"
```

### 3. Build and upload
- Through PlatformIO menu, select Build
- Once that is done and device is connected, select Upload and Monitor



## Sending Commands via MQTT

Go to the HiveMQ Cloud web client, connect using your credentials, and publish
to topic ota/update with one of these messages:
| Message | Description |
|---|---|
| `check_update` | Fetch version.txt from GitHub and update if newer version found |
| `fetch` | Force fetch both binaries from GitHub regardless of version |
| `verify` | Print size and first 16 bytes of stored binaries to serial monitor |
| `flash` | Blink onboard LED 5 times (connectivity test) |

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


