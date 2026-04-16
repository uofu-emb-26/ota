#ifndef OTA_APP_HELPER_H
#define OTA_APP_HELPER_H

#include <stdint.h>
#include "ota_config.h"

/* Determine which slot the calling code is executing from. */
static inline uint8_t get_current_slot(void)
{
    uint32_t fn_addr = (uint32_t)get_current_slot;

    if (fn_addr >= OTA_SLOT_A_START && fn_addr < OTA_SLOT_A_START + OTA_SLOT_A_SIZE) {
        return OTA_SLOT_A;
    }
    if (fn_addr >= OTA_SLOT_B_START && fn_addr < OTA_SLOT_B_START + OTA_SLOT_B_SIZE) {
        return OTA_SLOT_B;
    }

    return OTA_SLOT_NONE;
}

#endif /* OTA_APP_HELPER_H */
