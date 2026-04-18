#ifndef OTA_IMAGE_INFO_H
#define OTA_IMAGE_INFO_H

#include <stdint.h>
#include "ota_config.h"

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint32_t firmware_version;
    uint32_t image_size;
    uint32_t image_crc;
} ota_image_info_t;

extern const ota_image_info_t g_ota_image_info;

static inline const ota_image_info_t *ota_image_info_from_slot(uint32_t slot_base)
{
    return (const ota_image_info_t *)(slot_base + OTA_IMAGE_INFO_OFFSET);
}

static inline const ota_image_info_t *ota_get_running_image_info(void)
{
    return &g_ota_image_info;
}

#endif /* OTA_IMAGE_INFO_H */