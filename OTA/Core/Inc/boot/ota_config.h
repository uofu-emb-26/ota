#ifndef OTA_CONFIG_H
#define OTA_CONFIG_H

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Flash geometry  (STM32F072xB  – 128 KB, 2 KB pages)
 * ---------------------------------------------------------------------------
 *  0x08000000 ──────────────── Bootloader  16 KB (8 pages)
 *  0x08004000 ──────────────── Slot A      54 KB (27 pages)
 *  0x08011800 ──────────────── Slot B      54 KB (27 pages)
 *  0x0801F000 ──────────────── Scratch      2 KB (1 page)
 *  0x0801F800 ──────────────── Metadata     2 KB (1 page)
 *  0x08020000 ──────────────── end
 * -------------------------------------------------------------------------*/

#define OTA_FLASH_PAGE_SIZE         0x800U          /* 2 KB */

#define OTA_BOOTLOADER_START        0x08000000U
#define OTA_BOOTLOADER_SIZE         0x4000U         /* 16 KB – 8 pages  */

#define OTA_SLOT_A_START            0x08004000U
#define OTA_SLOT_A_SIZE             0xD800U         /* 54 KB – 27 pages */

#define OTA_SLOT_B_START            0x08011800U
#define OTA_SLOT_B_SIZE             0xD800U         /* 54 KB – 27 pages */

#define OTA_SCRATCH_START           0x0801F000U
#define OTA_SCRATCH_SIZE            0x800U          /* 2 KB  – 1 page   */

#define OTA_METADATA_START          0x0801F800U
#define OTA_METADATA_SIZE           0x800U          /* 2 KB  – 1 page   */

/* ---------------------------------------------------------------------------
 * SRAM layout
 *
 * The bootloader copies the active app's interrupt vector table (64 entries,
 * 256 bytes) to the first 256 bytes of SRAM immediately before jumping.
 * SYSCFG is then configured to alias SRAM at 0x00000000 so that the
 * Cortex-M0 (which has no SCB->VTOR) dispatches interrupts to the app's
 * handlers.  Both bootloader and application linker scripts start their
 * .data/.bss regions at OTA_APP_RAM_ORIGIN to leave this area untouched.
 * -------------------------------------------------------------------------*/
#define OTA_VECTOR_MIRROR_SRAM_ADDR 0x20000000U
#define OTA_VECTOR_TABLE_ENTRIES    64U             /* 16 core + 48 peripheral */
#define OTA_VECTOR_TABLE_SIZE       (OTA_VECTOR_TABLE_ENTRIES * 4U) /* 256 B */

#define OTA_APP_RAM_ORIGIN          0x20000100U     /* first usable RAM byte */
#define OTA_APP_RAM_END             0x20004000U     /* top of SRAM           */

/* ---------------------------------------------------------------------------
 * Metadata constants
 * -------------------------------------------------------------------------*/
#define OTA_METADATA_MAGIC          0xAB5A5AF0U
#define OTA_METADATA_FORMAT_VER     0x01U

/* Slot identifiers */
#define OTA_SLOT_NONE               0x00U
#define OTA_SLOT_A                  0x01U
#define OTA_SLOT_B                  0x02U

/* Image flag bits */
#define OTA_FLAG_VALID              0x01U  /* image has been programmed & CRC checked */
#define OTA_FLAG_CONFIRMED          0x02U  /* app has called ota_confirm_current_slot() */

/* Maximum number of consecutive unconfirmed boots before rollback */
#define OTA_MAX_TRIAL_BOOTS         3U

#endif /* OTA_CONFIG_H */
