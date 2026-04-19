# OTA Project Handover

## Purpose

This repository implements a software-managed A/B OTA foundation for STM32F072xB.

Current architecture:

- A small custom bootloader at flash start.
- One shared application codebase, built twice (Slot A and Slot B link addresses).
- Per-slot application image manifest stored inside each app image.
- Application is expected to receive future OTA payloads (UART3 path owned by another developer) and write to dormant slot.

~~The bootloader decides which slot to boot on reset based on metadata.~~

Current state:

- Bootloader currently chooses slot from runtime slot probing only (vector table + image manifest checks + firmware version compare).
- Metadata module still exists and is power-fail-safe, but is currently bypassed by active boot selection logic.

## Platform and Constraints

### MCU

- Part family: STM32F072xB
- Flash size: 128 KB
- RAM size: 16 KB
- Flash page size: 2 KB (`0x800`)

### Hardware constraints

1. Single-bank flash
- No hardware dual-bank swap.
- Running app must only erase/program dormant slot.

2. Cortex-M0 vector handling
- No VTOR usage in this architecture.
- Bootloader copies selected app vectors to SRAM and remaps SRAM to `0x00000000`.

3. Flash erase granularity
- Page erase only (2 KB).

## Current Flash Layout

- Bootloader: `0x08000000` to `0x08003FFF` (16 KB)
- Slot A: `0x08004000` to `0x080117FF` (54 KB)
- Slot B: `0x08011800` to `0x0801EFFF` (54 KB)
- Scratch: `0x0801F000` to `0x0801F7FF` (2 KB)
- Metadata: `0x0801F800` to `0x0801FFFF` (2 KB)

RAM reservation:

- `0x20000000` to `0x200000BF` is currently used for vector mirror (48 entries = 192 bytes).
- Linker scripts keep application RAM start at `0x20000100`, so mirror headroom remains safe.

## Current Repository Architecture

### Bootloader files

- `Core/Inc/boot/bootloader.h`
- `Core/Src/boot/bootloader.c`
- `Core/Src/boot/bootloader_main.c`
- `Core/Src/boot/bootloader_it.c`

Responsibilities today:

- Probe Slot A and Slot B runtime validity.
- Select highest valid firmware version from image manifests.
- Copy vector table to SRAM mirror.
- Remap SRAM and jump to selected app.

~~Read metadata and run pending/confirmed/trial rollback policy in the boot path.~~

### Metadata and layout files

- `Core/Inc/boot/ota_config.h`
- `Core/Inc/boot/ota_metadata.h`
- `Core/Src/common/ota_metadata.c`

Responsibilities:

- Partition and slot constants.
- Metadata structure, CRC, and scratch-page journal write flow.
- App-callable APIs:
  - `ota_confirm_current_slot()`
  - `ota_mark_slot_pending(slot_id)`

### Image manifest files

- `Core/Inc/common/ota_image_info.h`
- `Core/Src/common/ota_image_info.c`
- Linker placement in:
  - `STM32F072_APP_A.ld`
  - `STM32F072_APP_B.ld`

Manifest fields currently include:

- magic
- format version
- header size
- firmware version
- image size
- image CRC placeholder (`OTA_IMAGE_INFO_CRC_NONE`)

### Application-side helpers

- `Core/Inc/common/ota_app_helper.h`

Current helper APIs:

- `get_current_slot()`
- `get_dormant_slot()`

### Flash helper files

- `Core/Inc/common/flash_update.h`
- `Core/Src/common/flash_update.c`

## Build Targets and Artifacts

Active targets:

- `OTA_bootloader`
- `OTA_app_a`
- `OTA_app_b`

~~`OTA` (legacy single-image app target; not part of the A/B OTA flow)~~

Notes:

- App A and App B now share one firmware version define (`OTA_APP_FIRMWARE_VERSION` in `CMakeLists.txt`).

Recommended build commands:

```bash
cmake --preset Debug -B build/Debug
cmake --build build/Debug --target OTA_bootloader
cmake --build build/Debug --target OTA_app_a
cmake --build build/Debug --target OTA_app_b
```

## Current Implemented Behavior

### Boot flow (as of now)

1. Bootloader probes both slots.
2. Slot considered bootable if:
- vector table sanity passes
- manifest magic/version/header fields pass
- manifest image size is in slot bounds
3. If both slots bootable, higher `firmware_version` wins.
4. Bootloader remaps vectors and jumps.
5. If neither slot is bootable, returns `BOOT_ERR_NO_VALID_SLOT`.

~~Boot selection currently uses metadata pending/confirmed/trial state.~~

### Confirmation flow

- App currently calls `ota_confirm_current_slot()` early in startup.
- This is valid for bring-up but still a placeholder for final project milestone.

## Metadata Role: Current vs Target

### Current state

- Metadata storage is implemented and power-fail-safe.
- Metadata is **not** used by active boot selection right now.

### Intended steady-state role (agreed)

- Image manifest is source-of-truth for image identity and version.
- Metadata is source-of-truth for OTA workflow state.

Manifest should own:

- image version
- image size
- image integrity fields

Metadata should own:

- active slot
- pending slot
- confirmed slot
- trial boot count
- workflow flags

Read/write ownership guidelines:

- Bootloader: read metadata always, write boot-state transitions.
- Application: write only through narrow APIs (`ota_confirm_current_slot`, `ota_mark_slot_pending`), not raw struct mutations.

## What Is Outdated

~~Bootloader policy is currently metadata-driven with rollback active in boot path.~~

~~Slot image header format is not yet defined.~~

~~Post-build `.hex` generation is missing.~~

~~Helpers to determine current and inactive slot are missing.~~

~~Metadata API to mark slot pending is missing.~~

## Current Gaps / Not Yet Implemented

1. UART3 image reception and protocol framing.
2. Application-side full dormant-slot write state machine.
3. Final image CRC calculation and verification (manifest currently uses CRC placeholder).
4. Bootloader recovery behavior UX when no valid slot exists (for example UART diagnostics or recovery mode).
5. Security features: signatures/authenticity, anti-rollback policy.
6. Reintroduction of metadata-driven pending/confirmed trial-boot policy in bootloader (using metadata only for workflow state, not image identity).

## Planned Development Path

### Phase 1: Keep current runtime image-based boot valid (done)

1. Preserve manifest checks in bootloader as mandatory slot gate.
2. Add explicit UART debug messages in slot probe path for rejection reasons.

### Phase 2: Complete manifest integrity (done)

1. Populate `image_crc` during build or post-build tooling.
2. Verify CRC in bootloader probe before marking slot bootable.
3. Verify CRC in app updater flow before calling `ota_mark_slot_pending`.

### Phase 3: Re-enable metadata workflow in bootloader (under construction)

1. Use metadata only for pending/confirmed/trial logic.
2. Keep slot validity and version discovery strictly manifest-based.
3. Trial flow target:
- boot pending if valid
- app confirms
- rollback after trial limit when unconfirmed

### Phase 4: UART3 updater integration (Jeff)

1. Running app identifies dormant slot via helper.
2. Receive and write dormant-slot image.
3. Validate written image.
4. Call `ota_mark_slot_pending(dormant_slot)`.
5. Reboot.

### Phase 5: Confirmation timing hardening

1. Move confirmation call from early startup to health checkpoint.
2. Confirm only after required peripherals and update-critical services are healthy.

## Suggested Immediate TODO List

1. Add bootloader UART diagnostics for slot rejection reasons.
2. Implement real image CRC generation and probe-time verification.
3. Implement app-side dormant-slot write pipeline over UART3.
4. Reintroduce metadata-driven trial/rollback in bootloader with manifest as source of truth for image identity.
5. Add end-to-end tests:
- A valid, B valid (version compare)
- A valid, B empty
- B valid, A empty
- pending slot trial then confirm
- pending slot trial then rollback
- corrupted metadata recovery via scratch
- corrupted manifest or invalid vector rejection

## Operational Notes

1. Prefer ELF for debug/programming.
2. HEX is address-aware and safe for external tools.
3. BIN requires explicit destination address and also contains explicit Flash address. We need two unique `.bin` files for each slot.
4. Flash from current build outputs only; stale artifacts can mismatch manifest constants.

## Summary

Current mental model:

- bootloader is small and slot-runtime-aware
- app images are self-describing via manifest
- metadata is retained for OTA workflow persistence
- app transport/updater logic remains outside bootloader

Target mental model:

- manifest = image truth
- metadata = workflow truth