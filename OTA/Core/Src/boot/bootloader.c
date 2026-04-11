#include "bootloader.h"
#include "ota_metadata.h"
#include "ota_config.h"
#include "stm32f0xx_hal.h"
#include <string.h>

/* --------------------------------------------------------------------------
 * bootloader_select_slot
 *
 * Policy:
 *   1. If a pending slot exists (new OTA image waiting), use it and enter
 *      trial-boot mode.
 *   2. Otherwise use the confirmed (last known-good) slot.
 *   3. If the trial boot count has exceeded OTA_MAX_TRIAL_BOOTS, rollback
 *      to the confirmed slot.
 *   4. If the active slot image is not flagged VALID, try the other slot.
 *   5. If nothing is bootable, return OTA_SLOT_NONE.
 * --------------------------------------------------------------------------*/
uint8_t bootloader_select_slot(const ota_metadata_t *meta)
{
    uint8_t a_valid = (meta->slot_a_flags & OTA_FLAG_VALID) != 0U;
    uint8_t b_valid = (meta->slot_b_flags & OTA_FLAG_VALID) != 0U;

    /* Rollback check: too many un-confirmed boots */
    if ((meta->pending_slot != OTA_SLOT_NONE) &&
        (meta->trial_boot_count >= OTA_MAX_TRIAL_BOOTS)) {
        /* Pending slot failed to confirm – fall back to last confirmed */
        if (meta->confirmed_slot == OTA_SLOT_A && a_valid) {
            return OTA_SLOT_A;
        }
        if (meta->confirmed_slot == OTA_SLOT_B && b_valid) {
            return OTA_SLOT_B;
        }
        /* Confirmed slot is also gone – try anything valid */
        if (a_valid) { return OTA_SLOT_A; }
        if (b_valid) { return OTA_SLOT_B; }
        return OTA_SLOT_NONE;
    }

    /* Normal path: prefer pending, then active */
    uint8_t candidate = (meta->pending_slot != OTA_SLOT_NONE)
                        ? meta->pending_slot
                        : meta->active_slot;

    if (candidate == OTA_SLOT_A && a_valid) { return OTA_SLOT_A; }
    if (candidate == OTA_SLOT_B && b_valid) { return OTA_SLOT_B; }

    /* Candidate is not valid – try the other slot */
    if (candidate != OTA_SLOT_A && a_valid) { return OTA_SLOT_A; }
    if (candidate != OTA_SLOT_B && b_valid) { return OTA_SLOT_B; }

    return OTA_SLOT_NONE;
}

/* --------------------------------------------------------------------------
 * bootloader_run
 *
 * Reads metadata, decides boot slot, increments trial counter if in trial
 * mode, then jumps.  Returns only on unrecoverable error.
 * --------------------------------------------------------------------------*/
boot_result_t bootloader_run(void)
{
    ota_metadata_t meta;
    ota_metadata_read(&meta);   /* always returns a usable struct */

    uint8_t slot = bootloader_select_slot(&meta);
    if (slot == OTA_SLOT_NONE) {
        return BOOT_ERR_NO_VALID_SLOT;
    }

    /* If this is a trial boot (pending slot), increment the attempt counter
     * and persist it so the bootloader knows whether to roll back after a
     * reset without confirmation. */
    if (meta.pending_slot != OTA_SLOT_NONE &&
        slot              == meta.pending_slot) {
        meta.trial_boot_count++;
        meta.active_slot  = slot;
        meta.pending_slot = OTA_SLOT_NONE;  /* consumed */
        meta.sequence++;
        ota_metadata_write(&meta);
    }

    uint32_t slot_base = (slot == OTA_SLOT_A) ? OTA_SLOT_A_START
                                               : OTA_SLOT_B_START;

    bootloader_jump_to_app(slot_base);  /* never returns on success */

    return BOOT_ERR_NO_VALID_SLOT;
}

/* --------------------------------------------------------------------------
 * do_jump  –  naked trampoline; sets MSP then branches.
 *
 * The Cortex-M0 AAPCS passes the first argument in r0 and the second in r1.
 * The "naked" attribute suppresses the function prologue/epilogue so no
 * stack frame is created between the msr and bx instructions.
 * --------------------------------------------------------------------------*/
__attribute__((naked, noreturn))
static void do_jump(uint32_t new_sp, uint32_t new_pc)
{
    __asm volatile (
        "msr  msp, r0   \n"   /* set main stack pointer    */
        "dsb            \n"   /* complete any pending write */
        "isb            \n"   /* flush pipeline             */
        "bx   r1        \n"   /* branch (clears thumb-bit)  */
        :
        : "r"(new_sp), "r"(new_pc)
    );
}

/* --------------------------------------------------------------------------
 * bootloader_jump_to_app
 *
 * On Cortex-M0 there is no SCB->VTOR register.  The hardware always fetches
 * the interrupt vector table from the alias region 0x00000000.  By default
 * that alias points to main flash (0x08000000).
 *
 * To redirect interrupts to the app's handlers we:
 *   1. Copy the app's entire vector table from flash to SRAM at 0x20000000.
 *   2. Set SYSCFG MEM_MODE = 11  →  SRAM aliased at 0x00000000.
 *   3. Set MSP from app vector-table word 0 and branch to word 1.
 *
 * Both bootloader and app linker scripts start .data/.bss at 0x20000100 so
 * startup code never overwrites the 256-byte mirror at 0x20000000.
 * --------------------------------------------------------------------------*/
void bootloader_jump_to_app(uint32_t slot_base)
{
    const uint32_t *app_vectors   = (const uint32_t *)slot_base;
          uint32_t *sram_vectors  = (uint32_t *)OTA_VECTOR_MIRROR_SRAM_ADDR;

    /* Validate the app's stack pointer: must be within SRAM */
    uint32_t app_sp = app_vectors[0];
    if (app_sp < 0x20000000U || app_sp > OTA_APP_RAM_END) {
        return;   /* invalid – do not jump */
    }

    /* Validate the app's reset handler: must lie within the slot's flash range */
    uint32_t slot_size = (slot_base == OTA_SLOT_A_START) ? OTA_SLOT_A_SIZE
                                                          : OTA_SLOT_B_SIZE;
    uint32_t app_reset = app_vectors[1] & ~1U;  /* strip thumb bit for range check */
    if (app_reset < slot_base || app_reset >= slot_base + slot_size) {
        return;   /* invalid – do not jump */
    }

    /* Disable all interrupts before manipulating the vector table */
    __disable_irq();

    /* 1. Copy the app's vector table to the SRAM mirror location */
    for (uint32_t i = 0U; i < OTA_VECTOR_TABLE_ENTRIES; i++) {
        sram_vectors[i] = app_vectors[i];
    }

    /* Memory barrier: ensure all writes to SRAM complete
     * before SYSCFG remap takes effect */
    __DSB();

    /* 2. Remap SRAM to 0x00000000 via SYSCFG.
     *    __HAL_SYSCFG_REMAPMEMORY_SRAM sets MEM_MODE[1:0] = 11. */
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SRAM();

    __DSB();
    __ISB();
    __enable_irq();

    /* 3. Set MSP and branch – use the reset vector stored in the
     *    now-updated SRAM mirror so the Thumb bit is preserved. */
    do_jump(sram_vectors[0], sram_vectors[1]);

    /* Never reached */
    while (1) {}
}
