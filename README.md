# OTA Firmware Update Project

Reliable A/B firmware update foundation for STM32 with an ESP32 transport bridge.
Here is the final presentation video showing a demo and explenation of everything in this readme and more:

## DEMO VIDEOS:
https://youtu.be/UHACsZe9ke4
https://youtube.com/shorts/yXXsLTlftgA?feature=share

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
- An ESP32 transport companion is integrated to deliver update payloads to the STM32 application.

### What the final system should look like

1. A new firmware binary file is delivered over the network to the ESP32 bridge.
2. The running STM32 app receives the payload over UART and writes to the dormant slot.
3. The app marks the new slot as pending and reboots.
4. The bootloader validates integrity, CRC, and boots the new image.

### FSMs for the system are seen below, first is the Bootloader implementation FSM, and the second is the UART ESP32 STM32 protocol

![Bootloader_Flowchart_update_2.drawio.png](OTA/Bootloader_Flowchart_update_2.drawio.png)
![Arduino_state_machine.drawio_4.png](OTA/Arduino_state_machine.drawio_4.png)

## Project Organization

This repository is organized into two layers:
- STM32 firmware workspace in [OTA](OTA) with bootloader, Slot A, and Slot B builds.
- ESP32 firmware workspace in [OTA/ota_esp32](OTA/ota_esp32) with PlatformIO on Arduino framework


## System Architecture

End-to-end update path:

1. New firmware images are stored on https://github.com/anton2uha/OTAfiles.
2. HiveMQ sends a fetch request to the ESP32, which then downloads the above binaries to the flash in the ESP32. 
3. STM32 application receives OTA payload from the ESP32 through the communication protocol defined in the FSM above, and writes it to flash in the non-active flash bank.
4. Application marks dormant slot as pending and resets.
5. Bootloader validates available slots and selects boot target with the highest version.
6. New app image confirms itself after health checks.

## Hardware Interfaces

### STM32

- `UART3`: OTA data path from ESP32 to STM32. - Pins PC4,PC5 

### ESP32

- `UART0`: USB serial to receive new programs via the PlatformIO dev platform we used to program the ESP32.
- `UART2`: binary transfer path toward STM32 OTA updater. RX2,TX2



## Current Implementation Status

Implemented:

- A/B flash partitioning and custom bootloader runtime.
- Runtime slot probing with vector checks, manifest checks, CRC checks, and version compare.
- Shared app source built into two linked images (`Slot A`, `Slot B`).
- Power-fail-safe metadata persistence with scratch-page journaling.
- Post-build image CRC stamping and generation of `ELF`, `BIN`, `HEX`, and `MAP` artifacts.
- STM32 app-side `UART3` framed OTA receive/write pipeline.
- ESP32 protocol bridge integration.


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

## Authors
 
- Sameeran Chandorkar  
- Jeffrey Hansen  
- Nick Baret  
- Anthony Lesik 
