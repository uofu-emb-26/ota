#include "ota_image_info.h"

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION 0U
#endif

extern const uint32_t __ota_image_start;
extern const uint32_t __ota_image_end;
extern const uint32_t __ota_image_size;

__attribute__((section(".ota_image_info"), used))
const ota_image_info_t g_ota_image_info = {
    .magic = OTA_IMAGE_INFO_MAGIC,
    .format_version = OTA_IMAGE_INFO_FORMAT_VER,
    .header_size = (uint16_t)sizeof(ota_image_info_t),
    .firmware_version = (uint32_t)FIRMWARE_VERSION,
    .image_size = (uint32_t)(uintptr_t)&__ota_image_size,
    .image_crc = OTA_IMAGE_INFO_CRC_NONE,
};