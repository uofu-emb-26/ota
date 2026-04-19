# OTA Firmware Update Project

Reliable A/B firmware update foundation for STM32 with an ESP32 transport bridge.

## Table of Contents

- [Project Overview](#project-overview)
- [System Architecture](#system-architecture)
- [Hardware Interfaces](#hardware-interfaces)
- [Current Implementation Status](#current-implementation-status)
- [Repository Layout](#repository-layout)
- [Getting Started](#getting-started)
- [Roadmap](#roadmap)
- [Authors](#authors)

## Project Overview

### Problem this project solves

Many embedded products are deployed in places where physical reflashing is slow, costly, or impossible. A failed firmware update can also brick a device if there is no recovery path.

### Short description

This project builds a robust over-the-air update pipeline for an STM32 platform using an A/B swap firmware update layout. The design keeps one known-good binary while writing a new binary to the inactive slot, so updates can be validated before becoming permanent. The update process is also concurrent so the main application tasks are not halted during new binary download operation.

### Implementation overview

- A custom STM32 bootloader validates candidate app binaries at boot.
- The same application is built twice into Slot A and Slot B with custom known binary information.
- OTA update workflow statuses and metadata (pending/confirmed/trial state) is stored in flash with power-fail-safe writes.
- An ESP32 transport companion is being integrated to deliver update payloads to the STM32 application.

### What the final system should look like

1. A new firmware image is delivered over the network to the ESP32 bridge.
2. The running STM32 app receives the payload over UART and writes only the dormant slot.
3. The app marks the new slot as pending and reboots.
4. The bootloader validates integrity and boots the new image.
5. The new app confirms health; if it does not, rollback policy restores the last confirmed image.

## Project Organization

This repository is organized into two layers:

- Top-level documentation (this file) for quick onboarding.
- STM32 firmware workspace in [OTA](OTA) with bootloader, Slot A, and Slot B builds.
- ESP32 firmware workspace in [OTA/ota_esp32](OTA/ota_esp32) with PlatformIO on Arduino framework



## System Architecture

Planned end-to-end update path:

1. New firmware image is produced and staged by host/server tooling.
2. ESP32 bridge receives image stream.
3. STM32 application receives OTA payload and writes only the dormant slot.
4. Application marks dormant slot as pending and resets.
5. Bootloader validates available slots and selects boot target.
6. New app image confirms itself after health checks.

## Hardware Interfaces

### STM32

- `UART3`: OTA data path from ESP32 to STM32 application updater (planned).
- `UART4`: debug strings/diagnostics (current).

### ESP32

- `UART0`: binary transfer path toward STM32 OTA updater (planned).
- `UART2`: debug strings/diagnostics (planned).

Physical pin mapping is board-revision specific and should be finalized in project configuration.

## Current Implementation Status

Implemented:

- A/B flash partitioning and custom bootloader runtime.
- Runtime slot probing with vector checks, manifest checks, CRC checks, and version compare.
- Shared app source built into two linked images (`Slot A`, `Slot B`).
- Power-fail-safe metadata persistence with scratch-page journaling.
- Post-build image CRC stamping and generation of `ELF`, `BIN`, `HEX`, and `MAP` artifacts.

In progress / planned:

- STM32 app-side `UART3` framed OTA receive/write pipeline.
- ESP32 protocol bridge integration.
- Metadata-driven trial/rollback logic in active boot path.
- Security hardening (authenticity and anti-rollback).

## Repository Layout

- [OTA](OTA): STM32 firmware workspace.
- [OTA/README.md](OTA/README.md): technical README (file map, flash map, build/flash details).
- [OTA/PROJECT_HANDOVER.md](OTA/PROJECT_HANDOVER.md): detailed architecture and roadmap.
- [OTA/ota_esp32](OTA/ota_esp32): ESP32 companion workspace.

## Getting Started

For implementation details, build commands, and target-level docs:

1. Open [OTA/README.md](OTA/README.md)
2. Review [OTA/PROJECT_HANDOVER.md](OTA/PROJECT_HANDOVER.md)
3. Build from the `OTA` workspace using documented CMake targets

## Roadmap

The living engineering roadmap is maintained in [OTA/PROJECT_HANDOVER.md](OTA/PROJECT_HANDOVER.md).

Key near-term milestones:

1. Complete app-side dormant-slot update state machine.
2. Complete ESP32 transport path.
3. Re-enable metadata-driven trial/rollback in bootloader.
4. Add end-to-end update/recovery validation coverage.

## Authors
 
- Sameeran Chandorkar  
- Jeffrey Hansen  
- Nick Baret  
- Anthony Lesik 
