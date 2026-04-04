# OTA Project Handover

## Purpose

This repository is being evolved into a software-managed A/B OTA update system for STM32F072xB.

The target architecture is:

- A small custom bootloader at the start of flash.
- One shared application codebase.
- The same application codebase built twice:
  - once for Slot A
  - once for Slot B
- The currently running application receives a new image over a transport such as UART and writes it into the inactive slot.
- The bootloader decides which slot to boot on reset based on metadata.

This is intended to stay modular so the OTA policy and state machine can later be reused on other MCU families even if the platform-specific flash and boot handoff implementation changes.

## Platform Facts and Constraints

### MCU

- Part family in this repo: STM32F072xB
- Flash size: 128 KB
- RAM size: 16 KB
- Flash page size: 2 KB (`0x800`)

Confirmed from current HAL headers in this repository.

### Important hardware constraints

1. Single-bank flash
- There is no dual-bank hardware A/B swap support.
- A running image must never erase or reprogram the slot it is currently executing from.
- The application must always program the inactive slot.

2. Cortex-M0 vector handling
- STM32F072xB is Cortex-M0.
- This setup does not use `SCB->VTOR` for vector relocation.
- Instead, the bootloader copies the selected application's vector table into SRAM and remaps SRAM to address `0x00000000` using SYSCFG.

3. Flash erase granularity
- Erase is page-based at 2 KB.
- All partition sizes and metadata locations must be page-aligned.

4. Manufacturer ROM bootloader
- The factory bootloader in system memory is not the normal OTA mechanism here.
- It should be treated as a recovery/manufacturing path, not the main OTA flow.

## Design Decisions Made

### 1. Active-slot switching, not physical swap

The design chosen is A/B active-slot switching, not a physical copy-swap algorithm.

Meaning:

- Slot A and Slot B are fixed flash regions.
- The new image is written into the inactive slot.
- Metadata tells the bootloader which slot to try next.
- After reboot, the bootloader either confirms the new slot or rolls back.

Reasoning:

- Physical image swapping is more complex and dangerous on single-bank flash.
- Fixed slots are simpler, safer, and more portable.
- This still gives the core A/B behavior expected from OTA systems.

### 2. One application codebase, two slot-linked builds

There are not supposed to be two separate application implementations.

Instead:

- the same application source files are built into Slot A image
- the same application source files are built into Slot B image

Reasoning:

- This is the standard A/B firmware pattern.
- The distinction between A and B is a linker/build concern, not a source tree concern.

### 3. Bootloader stays small

The bootloader is intentionally limited to:

- reading metadata
- selecting the boot slot
- rollback behavior
- vector remap and jump

The transport logic such as UART OTA reception belongs in the application, not in the bootloader.

Reasoning:

- Smaller bootloader is easier to validate.
- The bootloader should change rarely.
- Update transport can evolve in the app without destabilizing first-stage boot.

### 4. Power-fail-safe metadata writes

Metadata writes use a scratch-page journal approach.

Reasoning:

- If power is lost after erasing metadata but before rewriting it, boot state would be lost.
- Writing first to scratch allows recovery on the next reset.

## Current Flash Layout

Implemented flash partitioning:

- Bootloader: `0x08000000` to `0x08003FFF` (16 KB)
- Slot A: `0x08004000` to `0x080117FF` (54 KB)
- Slot B: `0x08011800` to `0x0801EFFF` (54 KB)
- Scratch: `0x0801F000` to `0x0801F7FF` (2 KB)
- Metadata: `0x0801F800` to `0x0801FFFF` (2 KB)

RAM usage rule:

- `0x20000000` to `0x200000FF` is reserved for SRAM vector mirror.
- Bootloader and application linker scripts place `.data/.bss` from `0x20000100` upward.

## Current Repository Architecture

### Bootloader files

- `Core/Inc/bootloader.h`
- `Core/Src/bootloader.c`
- `Core/Src/bootloader_main.c`

Responsibilities:

- Read metadata
- Choose A or B
- Manage rollback policy
- Copy vector table to SRAM
- Remap SRAM to `0x00000000`
- Jump to selected application

### Metadata and layout files

- `Core/Inc/ota_config.h`
- `Core/Inc/ota_metadata.h`
- `Core/Src/ota_metadata.c`

Responsibilities:

- Partition constants
- Slot and flag definitions
- Metadata structure and CRC
- Scratch-page journal behavior
- Slot confirmation API

### Shared application files

- `Core/Src/main.c`
- `Core/Src/stm32f0xx_it.c`
- `Core/Src/stm32f0xx_hal_msp.c`
- `Core/Src/syscalls.c`
- `Core/Src/sysmem.c`
- plus normal application files under `Core/Src`

Responsibilities:

- Product logic
- Future OTA transport logic
- Future inactive-slot programming logic
- Slot confirmation after successful startup

### Flash helper files

- `Core/Inc/flash_update.h`
- `Core/Src/flash_update.c`

Responsibilities:

- Page erase
- Half-word programming
- Buffer programming

### Linker scripts

- `STM32F072_BOOTLOADER.ld`
- `STM32F072_APP_A.ld`
- `STM32F072_APP_B.ld`
- `STM32F072XX_FLASH.ld`

Notes:

- `STM32F072XX_FLASH.ld` is the legacy single-image linker script.
- The A/B OTA flow should use the slot-specific scripts.

### VS Code debug profiles

Configured in `.vscode/launch.json`:

- `Bootloader`
- `App A`
- `App B`

These profiles select the corresponding ELF explicitly.

## Current Build Targets

Relevant build targets:

- `OTA_bootloader`
- `OTA_app_a`
- `OTA_app_b`
- `OTA` (legacy single-image app target; not part of the A/B OTA flow)

Recommended commands:

```bash
cmake --preset Debug -B build/Debug
cmake --build build/Debug --target OTA_bootloader
cmake --build build/Debug --target OTA_app_a
cmake --build build/Debug --target OTA_app_b
```

Generated ELFs:

- `build/Debug/OTA_bootloader.elf`
- `build/Debug/OTA_app_a.elf`
- `build/Debug/OTA_app_b.elf`

Verified link addresses:

- `OTA_bootloader.elf` `.isr_vector` at `0x08000000`
- `OTA_app_a.elf` `.isr_vector` at `0x08004000`
- `OTA_app_b.elf` `.isr_vector` at `0x08011800`

This verification was done using `arm-none-eabi-readelf -S`.

## Current Implemented Behavior

### Boot flow

1. Bootloader starts from flash base.
2. Bootloader reads metadata.
3. Bootloader chooses a slot based on:
- active slot
- pending slot
- confirmed slot
- trial boot count
- valid flags
4. Bootloader copies the selected application's vector table into SRAM.
5. Bootloader remaps SRAM to `0x00000000`.
6. Bootloader loads MSP from the application's vector table and jumps to the application's reset handler.

### Confirmation flow

Current app code calls `ota_confirm_current_slot()` during startup in `main.c`.

This is only a placeholder strategy.

Production intent:

- confirm only after the application has passed meaningful startup/self-test checks
- not immediately at the top of `main()`

### Rollback model

- Metadata stores active/pending/confirmed state.
- If a pending image boots and is never confirmed within the allowed trial count, the bootloader is intended to roll back to the last confirmed slot.

## What Was Fixed During This Work

### Flash driver issues corrected

In `flash_update.c`:

1. Unlock logic bug fixed
- previous code checked lock state incorrectly against the status register
- corrected logic uses `FLASH->CR`

2. Page erase address programming bug fixed
- previous code used OR-assignment on `FLASH->AR`
- corrected to assignment

3. Error-returning interfaces added
- erase/program functions now return success/failure status

4. Bulk programming helper added
- `flash_write_buf()` added to support metadata and future OTA payload writes

## Artifact Format Guidance

### Why `.bin` may appear to start at address 0

A raw `.bin` file usually contains only bytes, not address metadata.

That means:

- the first byte in the file is file offset 0
- this does not mean the image is linked to flash address 0
- it only means the file itself is raw binary data

So yes: a `.bin` file often does not preserve the memory address information in the way ELF or Intel HEX does.

### How to verify the image location before flashing

Preferred checks:

1. ELF section addresses

Use:

```bash
arm-none-eabi-readelf -S build/Debug/OTA_app_a.elf
arm-none-eabi-readelf -S build/Debug/OTA_app_b.elf
arm-none-eabi-readelf -S build/Debug/OTA_bootloader.elf
```

Check `.isr_vector` and `.text` addresses.

2. Map files

Check:

- `build/Debug/OTA_bootloader.map`
- `build/Debug/OTA_app_a.map`
- `build/Debug/OTA_app_b.map`

These are authoritative for final placement.

3. Objdump or readelf program headers

These also preserve address information.

### Which file format to flash

Best choices:

1. ELF
- safest for debug/program flows
- contains section and symbol address information

2. Intel HEX
- also safe for programming
- preserves absolute addresses

Raw BIN is acceptable only if the programmer is explicitly told the correct base address.

Example rule:

- `OTA_app_a.bin` is just bytes for the Slot A-linked image
- programmer must be told to write them at `0x08004000`

### Practical recommendation

For VS Code debugging/programming:

- use the ELF files

For external programming tools:

- prefer ELF or HEX
- use BIN only when the write address is explicitly provided

## How The Repository Is Intended To Be Used

### Initial provisioning

Typical first flash:

1. Flash `OTA_bootloader.elf`
2. Flash `OTA_app_a.elf`
3. Leave Slot B empty until first update

### Normal OTA runtime model

If currently running Slot A:

1. Receive new image over UART
2. Erase/program Slot B
3. Validate the new image
4. Mark Slot B as pending in metadata
5. Reboot
6. Bootloader tries Slot B
7. Slot B confirms itself after successful startup

If currently running Slot B:

1. Receive new image over UART
2. Erase/program Slot A
3. Validate the new image
4. Mark Slot A as pending in metadata
5. Reboot
6. Bootloader tries Slot A
7. Slot A confirms itself after successful startup

Important rule:

- the active application must always write the inactive slot
- never erase or program the currently executing slot

### Application development model

You do not write two different applications.

You write one application codebase and build it twice:

- `OTA_app_a`
- `OTA_app_b`

The linker script determines whether the exact same source tree is placed in Slot A or Slot B.

## Current Gaps / Not Yet Implemented

The current repository still does not implement the complete OTA update flow.

Missing pieces:

1. UART image reception
2. Firmware image transport protocol
3. Slot image header format
4. Full slot image validation before setting valid flag
5. Application-side inactive-slot programming state machine
6. Metadata state transitions for real update requests
7. Better confirmation timing policy
8. Recovery-mode behavior when no valid slot exists
9. Strong authenticity/security checks such as signatures
10. Anti-rollback versioning policy

## Recommended Next Development Path

### Phase 1: Clean up build and artifact flow

1. Deprecate or remove the legacy `OTA` target from the A/B workflow
2. Add post-build generation of `.hex` and optional `.bin` files for:
- bootloader
- app A
- app B
3. Name artifacts clearly for programming and release usage

### Phase 2: Define image format

Create a firmware image header that includes at minimum:

- magic
- header version
- payload size
- payload CRC
- firmware version
- intended slot or generic slot compatibility indicator

Reasoning:

- Slot validity should not rely only on metadata.
- Each slot image should be self-describing and verifiable.

### Phase 3: Implement application-side OTA writer

Add an OTA write pipeline in the app that:

1. identifies the current slot
2. selects the inactive slot
3. erases the inactive slot region
4. receives firmware over UART in chunks
5. writes chunks to flash
6. computes CRC while receiving
7. verifies final image integrity
8. updates metadata to mark inactive slot as pending
9. triggers reboot

### Phase 4: Improve bootloader validation

Before jumping to a slot, the bootloader should validate:

1. vector table sanity
2. image header magic/version
3. image size bounds
4. CRC/hash if required at boot time

### Phase 5: Improve confirmation policy

Replace immediate `ota_confirm_current_slot()` call in `main.c` with a real policy.

Example:

- initialize clocks/peripherals
- run critical checks
- confirm only after UART stack / product functions are alive

### Phase 6: Add security

Longer-term additions:

1. signed update manifests
2. version monotonicity checks
3. anti-rollback counters
4. optional encryption if needed

## Suggested Immediate TODO List

1. Remove ambiguity around legacy target `OTA`
2. Add post-build `.hex` generation for all deployable images
3. Add helper functions to determine current slot and inactive slot inside the app
4. Add metadata API to mark slot pending and valid
5. Define image header format and place it at the start of each app image
6. Implement UART receive/write path into inactive slot
7. Add slot validation before boot and before marking metadata valid
8. Move confirmation call to a proper healthy-state checkpoint
9. Add end-to-end tests for:
- A confirmed to B pending to B confirmed
- B confirmed to A pending to A confirmed
- pending image reset without confirm causes rollback
- corrupted metadata recovery via scratch
- invalid vector table prevents jump

## Verification Guidance

### Build-time checks

Verify after each build:

- linker script used is correct
- `.isr_vector` address matches expected slot
- image size fits partition

### On-target checks

Suggested validation sequence:

1. Bootloader + App A initial bring-up
2. Confirm app starts correctly through bootloader
3. Inspect metadata page contents
4. Manually force pending Slot B and test jump
5. Verify rollback by intentionally skipping confirmation
6. Corrupt metadata CRC and verify fallback behavior

## Important Operational Notes

1. Debug/program using ELF when possible
2. Use the VS Code launch profiles `Bootloader`, `App A`, and `App B`
3. Avoid using the legacy `OTA.elf` for A/B testing
4. Use `.bin` only when you also control the destination flash address explicitly
5. Prefer `.hex` or `.elf` for tools that can consume address-aware formats

## Summary of Current Intent

The repository should evolve into:

- a stable, small bootloader
- a single reusable application codebase
- slot-specific builds for A and B
- application-managed update transport and inactive-slot programming
- bootloader-managed slot selection and rollback

That is the correct mental model for continuing development in future chat sessions.