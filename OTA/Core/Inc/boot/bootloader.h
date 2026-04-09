#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include "ota_metadata.h"

/* ---------------------------------------------------------------------------
 * Boot result codes
 * -------------------------------------------------------------------------*/
typedef enum {
    BOOT_OK               =  0,
    BOOT_ERR_NO_VALID_SLOT = -1,
    BOOT_ERR_FLASH         = -2,
} boot_result_t;

/* ---------------------------------------------------------------------------
 * Bootloader core API
 *
 * bootloader_run()
 *   Reads metadata, selects the boot slot, validates the image, relocates
 *   the interrupt vector table to SRAM, and jumps.  Never returns on success.
 *   Returns BOOT_ERR_* if no bootable image is found.
 *
 * bootloader_select_slot()
 *   Pure policy function – choose which slot to boot given current metadata.
 *   Returns OTA_SLOT_A, OTA_SLOT_B, or OTA_SLOT_NONE.
 *
 * bootloader_jump_to_app()
 *   Copies the app's interrupt vector table from flash to SRAM, configures
 *   SYSCFG to alias SRAM at 0x00000000 (Cortex-M0 has no SCB->VTOR), then
 *   branches to the app's Reset_Handler.  Never returns.
 * -------------------------------------------------------------------------*/
boot_result_t bootloader_run(void);
uint8_t       bootloader_select_slot(const ota_metadata_t *meta);
void          bootloader_jump_to_app(uint32_t slot_base);  /* no return */

#endif /* BOOTLOADER_H */
