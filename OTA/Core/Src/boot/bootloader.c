#include "bootloader.h"
#include "ota_image_info.h"
#include "ota_config.h"
#include "stm32f0xx_hal.h"
#include <string.h>

typedef struct {
    uint8_t slot_id;
    uint32_t slot_base;
    uint32_t slot_size;
    uint8_t bootable;
    ota_image_info_t image_info;
} boot_slot_info_t;

static uint8_t boot_slot_vector_is_valid(uint32_t slot_base)
{
    const uint32_t *app_vectors = (const uint32_t *)slot_base;
    uint32_t app_sp = app_vectors[0];
    uint32_t slot_size = (slot_base == OTA_SLOT_A_START) ? OTA_SLOT_A_SIZE
                                                          : OTA_SLOT_B_SIZE;
    uint32_t app_reset = app_vectors[1] & ~1U;

    if (app_sp < 0x20000000U || app_sp > OTA_APP_RAM_END) {
        return 0U;
    }

    if (app_reset < slot_base || app_reset >= slot_base + slot_size) {
        return 0U;
    }

    return 1U;
}

static void bootloader_probe_slot(uint8_t slot_id,
                                  uint32_t slot_base,
                                  uint32_t slot_size,
                                  boot_slot_info_t *slot)
{
    memset(slot, 0, sizeof(*slot));
    slot->slot_id = slot_id;
    slot->slot_base = slot_base;
    slot->slot_size = slot_size;

    if (boot_slot_vector_is_valid(slot_base) == 0U) {
        return;
    }

    const ota_image_info_t *manifest = ota_image_info_from_slot(slot_base);
    if (manifest->magic != OTA_IMAGE_INFO_MAGIC) {
        return;
    }
    if (manifest->format_version != OTA_IMAGE_INFO_FORMAT_VER) {
        return;
    }
    if (manifest->header_size < sizeof(ota_image_info_t)) {
        return;
    }
    if (manifest->image_size < (OTA_IMAGE_INFO_OFFSET + manifest->header_size)) {
        return;
    }
    if (manifest->image_size > slot_size) {
        return;
    }

    memcpy(&slot->image_info, manifest, sizeof(slot->image_info));
    slot->bootable = 1U;
}

static uint8_t bootloader_best_valid_slot(const boot_slot_info_t *slot_a,
                                          const boot_slot_info_t *slot_b)
{
    if (slot_a->bootable != 0U && slot_b->bootable != 0U) {
        if (slot_b->image_info.firmware_version > slot_a->image_info.firmware_version) {
            return OTA_SLOT_B;
        }
        return OTA_SLOT_A;
    }
    if (slot_a->bootable != 0U) {
        return OTA_SLOT_A;
    }
    if (slot_b->bootable != 0U) {
        return OTA_SLOT_B;
    }

    return OTA_SLOT_NONE;
}

/* --------------------------------------------------------------------------
 * bootloader_run
 *
 * Minimal runtime selector:
 *   - Probe both slots directly from flash (vector table + image_info)
 *   - If both are valid, boot the higher firmware version
 *   - If one is valid, boot it
 *   - If none are valid, return BOOT_ERR_NO_VALID_SLOT
 *
 * Metadata-based trial/rollback policy is intentionally bypassed for now.
 * --------------------------------------------------------------------------*/
boot_result_t bootloader_run(void)
{
    boot_slot_info_t slot_a;
    boot_slot_info_t slot_b;
    bootloader_probe_slot(OTA_SLOT_A, OTA_SLOT_A_START, OTA_SLOT_A_SIZE, &slot_a);
    bootloader_probe_slot(OTA_SLOT_B, OTA_SLOT_B_START, OTA_SLOT_B_SIZE, &slot_b);

    uint8_t slot = bootloader_best_valid_slot(&slot_a, &slot_b);
    if (slot == OTA_SLOT_NONE) {
        return BOOT_ERR_NO_VALID_SLOT;
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

    /* 3. Set MSP and branch – use the reset vector stored in the
     *    now-updated SRAM mirror so the Thumb bit is preserved. */
    do_jump(sram_vectors[0], sram_vectors[1]);

    /* Never reached */
    while (1) {}
}
