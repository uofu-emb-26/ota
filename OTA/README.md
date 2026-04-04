## What was built
_Last Update: April 3, 2026_

### New files
| File | Purpose |
|---|---|
| ota_config.h | All partition address constants, SRAM vector mirror layout, slot/flag defines |
| ota_metadata.h | 36-byte packed `ota_metadata_t`, CRC/read/write API, `ota_confirm_current_slot()` |
| ota_metadata.c | Software CRC32, scratch-page journal (power-fail safe metadata writes), slot confirmation |
| bootloader.h | `bootloader_run()`, `bootloader_select_slot()`, `bootloader_jump_to_app()` API |
| bootloader.c | A/B policy engine, SRAM vector-table copy, SYSCFG remap, `naked` assembly jump trampoline |
| bootloader_main.c | Minimal bootloader `main()` — HAL init → `bootloader_run()` |
| STM32F072_BOOTLOADER.ld | FLASH 0x08000000, 16 KB; RAM 0x20000100, 15 872 B |
| STM32F072_APP_A.ld | FLASH 0x08004000, 54 KB; RAM 0x20000100, 15 872 B |
| STM32F072_APP_B.ld | FLASH 0x08011800, 54 KB; same RAM layout |

### Modified files
| File | Change |
|---|---|
| flash_update.h | New interface: error returns, `flash_erase_page`, `flash_write_buf`, legacy alias |
| flash_update.c | Fixed two register bugs (`SR→CR` in unlock; `\|=→=` in AR assignment), full HAL-compatible rewrite |
| main.c | Added `ota_confirm_current_slot()` call; fixed ISR (unlock before erase) |
| CMakeLists.txt | `OTA_bootloader` target, per-target linker scripts and map flags |
| gcc-arm-none-eabi.cmake | Removed global `-T` and `-Map`; now per-target |

### Flash map in effect
| Region | Start | End | Size |
|---|---|---|---|
| Bootloader | `0x08000000` | `0x08003FFF` | 16 KB |
| Slot A | `0x08004000` | `0x080117FF` | 54 KB |
| Slot B | `0x08011800` | `0x0801EFFF` | 54 KB |
| Scratch | `0x0801F000` | `0x0801F7FF` | 2 KB |
| Metadata | `0x0801F800` | `0x0801FFFF` | 2 KB |

### Key design points
- **No `SCB->VTOR`** — Cortex-M0 has none; instead the bootloader copies the app's 64-entry interrupt vector table to SRAM then sets `SYSCFG MEM_MODE=11` to alias SRAM at `0x00000000`.
- **Power-fail safe** — metadata writes use a scratch-page journal: scratch is validated before the primary page is erased.
- **Rollback** — if an app boots `OTA_MAX_TRIAL_BOOTS` (3) times without calling `ota_confirm_current_slot()`, the next boot falls back to the last confirmed slot.
- **Portable** — the policy logic (`bootloader_select_slot`, `ota_metadata_*`) is pure C with no hardware dependencies; only `bootloader_jump_to_app` and flash_update.c are MCU-specific.