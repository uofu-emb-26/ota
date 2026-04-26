# OTA STM32 Firmware Workspace

Technical implementation README for the firmware workspace in this directory.

## Table of Contents

- [Overview](#overview)
- [Current Boot Behavior](#current-boot-behavior)
- [Hardware + Interface Plan](#hardware--interface-plan)
- [Memory Layout](#memory-layout)
- [Code Organization](#code-organization)
- [Build and Artifacts](#build-and-artifacts)
- [Flash and Run](#flash-and-run)
- [Status and Roadmap](#status-and-roadmap)

## Overview

This workspace implements a software-managed `A/B OTA` foundation for `STM32F072xB`.

Core components:

- Custom bootloader at flash base.
- Single shared application codebase linked twice (`Slot A`, `Slot B`).
- Per-slot image manifest (`ota_image_info`) embedded in each app image.
- Metadata persistence (`ota_metadata`) for OTA workflow state.

## Current Boot Behavior

Bootloader runtime currently uses image probing, not metadata policy, to choose the boot target.

Decision flow:

1. Probe `Slot A` and `Slot B`.
2. Reject slot if vector table sanity fails.
3. Reject slot if manifest checks fail (`magic`, `format`, `header_size`, `image_size`).
4. Reject slot if image CRC is missing or mismatched.
5. If both valid, boot higher `firmware_version`.
6. If one valid, boot that slot.
7. If none valid, return `BOOT_ERR_NO_VALID_SLOT` and remain in bootloader loop.

Implementation references:

- [Core/Src/boot/bootloader_main.c](Core/Src/boot/bootloader_main.c)
- [Core/Src/boot/bootloader.c](Core/Src/boot/bootloader.c)
- [Core/Inc/common/ota_image_info.h](Core/Inc/common/ota_image_info.h)

## Hardware + Interface Plan

### STM32

- `UART3`: planned OTA payload ingress from ESP32 updater.
- `UART4`: debug output path.

### ESP32 companion

- `UART0`: planned binary transfer path toward STM32 app updater.
- `UART2`: planned debug output path.

Companion workspace:

- [ota_esp32](ota_esp32)

## Memory Layout

| Region | Start | End | Size |
|---|---|---|---|
| Bootloader | `0x08000000` | `0x08003FFF` | `16 KB` |
| Slot A | `0x08004000` | `0x080117FF` | `54 KB` |
| Slot B | `0x08011800` | `0x0801EFFF` | `54 KB` |
| Scratch | `0x0801F000` | `0x0801F7FF` | `2 KB` |
| Metadata | `0x0801F800` | `0x0801FFFF` | `2 KB` |

RAM note:

- Vector mirror reserved at `0x20000000` to `0x200000BF`.
- App linker scripts place RAM start at `0x20000100`.

Relevant definitions:

- [Core/Inc/boot/ota_config.h](Core/Inc/boot/ota_config.h)
- [STM32F072_BOOTLOADER.ld](STM32F072_BOOTLOADER.ld)
- [STM32F072_APP_A.ld](STM32F072_APP_A.ld)
- [STM32F072_APP_B.ld](STM32F072_APP_B.ld)

## Code Organization

### Bootloader

- [Core/Inc/boot/bootloader.h](Core/Inc/boot/bootloader.h): bootloader API.
- [Core/Src/boot/bootloader.c](Core/Src/boot/bootloader.c): slot probe, validation, selection, jump.
- [Core/Src/boot/bootloader_main.c](Core/Src/boot/bootloader_main.c): bootloader entrypoint.
- [Core/Src/boot/bootloader_it.c](Core/Src/boot/bootloader_it.c): bootloader interrupt handlers.

### Metadata + workflow state

- [Core/Inc/boot/ota_metadata.h](Core/Inc/boot/ota_metadata.h): metadata layout and API.
- [Core/Src/common/ota_metadata.c](Core/Src/common/ota_metadata.c): CRC, scratch-journal read/write, confirm/pending APIs.

### App image manifest + helpers

- [Core/Inc/common/ota_image_info.h](Core/Inc/common/ota_image_info.h)
- [Core/Src/common/ota_image_info.c](Core/Src/common/ota_image_info.c)
- [Core/Inc/common/ota_app_helper.h](Core/Inc/common/ota_app_helper.h)

### Flash programming abstraction

- [Core/Inc/common/flash_update.h](Core/Inc/common/flash_update.h)
- [Core/Src/common/flash_update.c](Core/Src/common/flash_update.c)

### App entry

- [Core/Src/app/main.c](Core/Src/app/main.c): startup sequence and `ota_confirm_current_slot()` call.

## Build and Artifacts

Primary targets:

- `OTA_bootloader`
- `OTA_app_a`
- `OTA_app_b`

Build commands:

```bash
cmake --preset Debug -B build/Debug
cmake --build build/Debug --target OTA_bootloader
cmake --build build/Debug --target OTA_app_a
cmake --build build/Debug --target OTA_app_b
```

Generated outputs:

- `*.elf`: debug + programming input.
- `*.bin`: raw image payload.
- `*.hex`: address-aware image.
- `*.map`: memory usage map.

Build configuration and post-build tools:

- [CMakeLists.txt](CMakeLists.txt)
- [tools/ota_patch_image_crc.py](tools/ota_patch_image_crc.py)

## Flash and Run

Workspace tasks include:

- `Build All Targets`
- `Flash Bootloader`
- `Flash App A`
- `Flash App B`
- `Flash All Targets`
- `Erase Flash`

Manual command references:

- [flash_commands.txt](flash_commands.txt)

## Status and Roadmap

Current implementation and milestones are tracked in:

- [PROJECT_HANDOVER.md](PROJECT_HANDOVER.md)

Near-term work:

1. Complete app-side `UART3` receive/write pipeline.
2. Complete dormant-slot update state machine.
3. Re-enable metadata-driven trial/rollback policy in bootloader.
4. Add stronger recovery UX for no-valid-slot cases.
5. Add authenticity and anti-rollback security features.