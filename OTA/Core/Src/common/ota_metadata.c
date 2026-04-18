#include "ota_metadata.h"
#include "flash_update.h"
#include <string.h>

/* Compile-time layout check */
typedef char _check_metadata_size[(sizeof(ota_metadata_t) == 20U) ? 1 : -1];

/* --------------------------------------------------------------------------
 * CRC-32 (IEEE 802.3, reflected polynomial 0xEDB88320)
 * --------------------------------------------------------------------------*/
static uint32_t crc32_accumulate(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0xEDB88320U;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint32_t ota_crc32(const uint8_t *data, uint32_t len)
{
    return crc32_accumulate(0xFFFFFFFFU, data, len) ^ 0xFFFFFFFFU;
}

uint32_t ota_metadata_crc(const ota_metadata_t *meta)
{
    return ota_crc32((const uint8_t *)meta, offsetof(ota_metadata_t, metadata_crc));
}

/* --------------------------------------------------------------------------
 * Internal helpers
 * --------------------------------------------------------------------------*/
static int metadata_is_valid(const ota_metadata_t *meta)
{
    if (meta->magic != OTA_METADATA_MAGIC) {
        return 0;
    }
    if (meta->format_version != OTA_METADATA_FORMAT_VER) {
        return 0;
    }
    if (meta->metadata_crc != ota_metadata_crc(meta)) {
        return 0;
    }
    return 1;
}

/* Write a packed ota_metadata_t to the given flash page address.
 * The page must be erased before calling this function. */
static ota_meta_result_t write_metadata_to_page(uint32_t page_addr,
                                                 const ota_metadata_t *meta)
{
    int result = flash_write_buf(page_addr,
                                 (const uint8_t *)meta,
                                 (uint32_t)sizeof(ota_metadata_t));
    return (result == 0) ? OTA_META_OK : OTA_META_ERR_FLASH;
}

/* --------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------*/
void ota_metadata_init_default(ota_metadata_t *meta)
{
    memset(meta, 0, sizeof(ota_metadata_t));
    meta->magic          = OTA_METADATA_MAGIC;
    meta->format_version = OTA_METADATA_FORMAT_VER;
    meta->active_slot    = OTA_SLOT_A;
    meta->pending_slot   = OTA_SLOT_NONE;
    /* First-boot provisioning assumption:
     * Bootloader + App A are flashed at manufacturing time.
     * Mark Slot A as valid/confirmed so blank metadata still boots. */
    meta->confirmed_slot = OTA_SLOT_A;
    meta->slot_a_flags   = OTA_FLAG_VALID | OTA_FLAG_CONFIRMED;
    meta->slot_b_flags   = 0U;
    meta->sequence       = 0U;
    meta->trial_boot_count = 0U;
    meta->metadata_crc   = ota_metadata_crc(meta);
}

ota_meta_result_t ota_metadata_read(ota_metadata_t *out)
{
    const ota_metadata_t *meta_page =
        (const ota_metadata_t *)OTA_METADATA_START;
    const ota_metadata_t *scratch_page =
        (const ota_metadata_t *)OTA_SCRATCH_START;

    /* 1. Try primary metadata page first */
    if (metadata_is_valid(meta_page)) {
        memcpy(out, meta_page, sizeof(ota_metadata_t));
        return OTA_META_OK;
    }

    /* 2. Fall back to scratch page (handles interrupted write to metadata page) */
    if (metadata_is_valid(scratch_page)) {
        memcpy(out, scratch_page, sizeof(ota_metadata_t));
        /* Restore metadata page from scratch */
        if (flash_erase_page(OTA_METADATA_START) == 0) {
            write_metadata_to_page(OTA_METADATA_START, out);
        }
        return OTA_META_OK;
    }

    /* 3. No valid record found – use safe defaults */
    ota_metadata_init_default(out);
    return OTA_META_ERR_CRC;
}

ota_meta_result_t ota_metadata_write(const ota_metadata_t *in)
{
    ota_metadata_t tmp;
    memcpy(&tmp, in, sizeof(ota_metadata_t));
    tmp.metadata_crc = ota_metadata_crc(&tmp);

    /* -----------------------------------------------------------------------
     * Scratch-page journal protocol (power-fail safe):
     *   1. Erase scratch page
     *   2. Write new record to scratch page
     *   3. Verify scratch page CRC
     *   4. Erase metadata page
     *   5. Write new record to metadata page
     * If power fails at steps 1-3: metadata page still valid (old data).
     * If power fails at step 4-5: scratch page has valid data; ota_metadata_read()
     *   will recover by copying scratch → metadata.
     * -----------------------------------------------------------------------*/

    /* Step 1: erase scratch */
    if (flash_erase_page(OTA_SCRATCH_START) != 0) {
        return OTA_META_ERR_FLASH;
    }

    /* Step 2: write new record to scratch */
    if (write_metadata_to_page(OTA_SCRATCH_START, &tmp) != OTA_META_OK) {
        return OTA_META_ERR_FLASH;
    }

    /* Step 3: verify scratch CRC */
    const ota_metadata_t *scratch_verify =
        (const ota_metadata_t *)OTA_SCRATCH_START;
    if (!metadata_is_valid(scratch_verify)) {
        return OTA_META_ERR_FLASH;
    }

    /* Step 4: erase primary metadata page */
    if (flash_erase_page(OTA_METADATA_START) != 0) {
        return OTA_META_ERR_FLASH;
    }

    /* Step 5: write new record to metadata page */
    if (write_metadata_to_page(OTA_METADATA_START, &tmp) != OTA_META_OK) {
        return OTA_META_ERR_FLASH;
    }

    return OTA_META_OK;
}

ota_meta_result_t ota_confirm_current_slot(void)
{
    /* Determine which slot this binary is running in by inspecting its own
     * load address (link-time address of this function). */
    uint32_t this_fn = (uint32_t)ota_confirm_current_slot;
    uint8_t  running_slot;

    if (this_fn >= OTA_SLOT_A_START &&
        this_fn <  OTA_SLOT_A_START + OTA_SLOT_A_SIZE) {
        running_slot = OTA_SLOT_A;
    } else if (this_fn >= OTA_SLOT_B_START &&
               this_fn <  OTA_SLOT_B_START + OTA_SLOT_B_SIZE) {
        running_slot = OTA_SLOT_B;
    } else {
        /* Running in bootloader space or unknown – nothing to confirm */
        return OTA_META_OK;
    }

    ota_metadata_t meta;
    ota_metadata_read(&meta);   /* always populates meta with a usable struct */

    if (meta.confirmed_slot == running_slot &&
        meta.pending_slot != running_slot &&
        (meta.trial_boot_count == 0U)) {
        /* Already confirmed; preserve any staged update for the other slot */
        return OTA_META_OK;
    }

    /* Mark slot confirmed */
    if (running_slot == OTA_SLOT_A) {
        meta.slot_a_flags |= OTA_FLAG_CONFIRMED;
    } else {
        meta.slot_b_flags |= OTA_FLAG_CONFIRMED;
    }
    meta.confirmed_slot   = running_slot;
    meta.active_slot      = running_slot;
    meta.pending_slot     = OTA_SLOT_NONE;
    meta.trial_boot_count = 0U;
    meta.sequence++;

    return ota_metadata_write(&meta);
}

ota_meta_result_t ota_mark_slot_pending(uint8_t slot_id)
{
    if (slot_id != OTA_SLOT_A && slot_id != OTA_SLOT_B) {
        return OTA_META_ERR_PARAM;
    }

    ota_metadata_t meta;
    ota_metadata_read(&meta);

    if (meta.pending_slot == slot_id && meta.trial_boot_count == 0U) {
        return OTA_META_OK;
    }

    meta.pending_slot = slot_id;
    meta.trial_boot_count = 0U;
    meta.sequence++;

    return ota_metadata_write(&meta);
}
