#ifndef OTA_METADATA_H
#define OTA_METADATA_H

#include <stdint.h>
#include <stddef.h>
#include "ota_config.h"

/* ---------------------------------------------------------------------------
 * Persistent boot-state record stored in the dedicated metadata flash page.
 *
 * Layout is packed so sizeof(ota_metadata_t) == 36 bytes regardless of
 * compiler padding.  The metadata_crc field covers all preceding bytes:
 *   ota_metadata_crc(meta) == crc32(meta, offsetof(metadata_crc))
 * -------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* OTA_METADATA_MAGIC – detects erased page   */
    uint8_t  format_version;    /* OTA_METADATA_FORMAT_VER                    */
    uint8_t  active_slot;       /* Current boot target: OTA_SLOT_A / _B       */
    uint8_t  pending_slot;      /* Slot to try next:    OTA_SLOT_A / _B / NONE*/
    uint8_t  confirmed_slot;    /* Last confirmed (known-good) slot            */
    uint8_t  trial_boot_count;  /* Incremented each unconfirmed boot attempt  */
    uint8_t  slot_a_flags;      /* OTA_FLAG_VALID | OTA_FLAG_CONFIRMED        */
    uint8_t  slot_b_flags;      /* OTA_FLAG_VALID | OTA_FLAG_CONFIRMED        */
    uint8_t  _reserved;
    uint32_t slot_a_size;       /* Byte length of the image in Slot A         */
    uint32_t slot_b_size;       /* Byte length of the image in Slot B         */
    uint32_t slot_a_crc;        /* CRC32 of the entire Slot A image           */
    uint32_t slot_b_crc;        /* CRC32 of the entire Slot B image           */
    uint32_t sequence;          /* Monotonic counter – incremented each write */
    uint32_t metadata_crc;      /* CRC32 of all bytes above this field        */
} ota_metadata_t;
/* static_assert: sizeof == 36 is checked in ota_metadata.c */

typedef enum {
    OTA_META_OK         = 0,
    OTA_META_ERR_CRC    = -1,   /* CRC mismatch / erased page */
    OTA_META_ERR_FLASH  = -2,   /* Flash erase/write failure  */
} ota_meta_result_t;

/* ---------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------*/

/* Read and validate metadata from flash (tries metadata page then scratch). */
ota_meta_result_t ota_metadata_read(ota_metadata_t *out);

/* Atomically write metadata using a scratch-page journal for power-fail safety. */
ota_meta_result_t ota_metadata_write(const ota_metadata_t *in);

/* Fill *meta with safe defaults (Slot A active, no pending, unconfirmed). */
void              ota_metadata_init_default(ota_metadata_t *meta);

/* Compute CRC32 over all fields except metadata_crc itself. */
uint32_t          ota_metadata_crc(const ota_metadata_t *meta);

/* General-purpose CRC32 (IEEE 802.3, reflected poly 0xEDB88320). */
uint32_t          ota_crc32(const uint8_t *data, uint32_t len);

/* ---------------------------------------------------------------------------
 * App-side confirmation API
 *
 * Call this after the application has determined it has started successfully.
 * Sets OTA_FLAG_CONFIRMED on the active slot and clears trial_boot_count.
 * If this is never called after a pending-slot boot, the bootloader will
 * roll back to the previously confirmed slot on the next reset.
 * -------------------------------------------------------------------------*/
ota_meta_result_t ota_confirm_current_slot(void);

#endif /* OTA_METADATA_H */
