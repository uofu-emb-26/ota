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
