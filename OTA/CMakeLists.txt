cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME OTA)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

message(STATUS "Legacy target 'OTA' is deprecated and no longer built. Use OTA_bootloader, OTA_app_a, or OTA_app_b.")

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

set(OTA_APP_FIRMWARE_VERSION 100)

# ---------------------------------------------------------------------------
# Slot-specific application targets
# ---------------------------------------------------------------------------
set(OTA_APP_COMMON_SOURCES
    startup_stm32f072xb.s
    Core/Src/app/main.c
    Core/Src/app/stm32f0xx_it.c
    Core/Src/app/stm32f0xx_hal_msp.c
    Core/Src/common/sysmem.c
    Core/Src/common/syscalls.c
    Core/Src/common/flash_update.c
    Core/Src/common/ota_image_info.c
    Core/Src/common/ota_metadata.c
    Core/Src/common/uart_debug.c
    Core/Src/app/hal_gpio.c
    Core/Src/common/sysmem.c
    Core/Src/common/led.c
)

add_executable(OTA_app_a ${OTA_APP_COMMON_SOURCES})
target_link_libraries(OTA_app_a
    stm32cubemx
    STM32_Drivers
    m
)
target_compile_definitions(OTA_app_a PRIVATE FIRMWARE_VERSION=${OTA_APP_FIRMWARE_VERSION})
target_link_options(OTA_app_a PRIVATE
    -T "${CMAKE_SOURCE_DIR}/STM32F072_APP_A.ld"
    -Wl,-Map=OTA_app_a.map
    -Wl,--print-memory-usage
)
set_target_properties(OTA_app_a PROPERTIES
    LINK_DEPENDS "${CMAKE_SOURCE_DIR}/STM32F072_APP_A.ld"
    ADDITIONAL_CLEAN_FILES "OTA_app_a.map;OTA_app_a.hex;OTA_app_a.bin"
)

add_executable(OTA_app_b ${OTA_APP_COMMON_SOURCES})
target_link_libraries(OTA_app_b
    stm32cubemx
    STM32_Drivers
    m
)
target_compile_definitions(OTA_app_b PRIVATE FIRMWARE_VERSION=${OTA_APP_FIRMWARE_VERSION})
target_link_options(OTA_app_b PRIVATE
    -T "${CMAKE_SOURCE_DIR}/STM32F072_APP_B.ld"
    -Wl,-Map=OTA_app_b.map
    -Wl,--print-memory-usage
)
set_target_properties(OTA_app_b PROPERTIES
    LINK_DEPENDS "${CMAKE_SOURCE_DIR}/STM32F072_APP_B.ld"
    ADDITIONAL_CLEAN_FILES "OTA_app_b.map;OTA_app_b.hex;OTA_app_b.bin"
)

# ---------------------------------------------------------------------------
# Bootloader target
# ---------------------------------------------------------------------------
add_executable(OTA_bootloader
    startup_stm32f072xb.s
    Core/Src/common/flash_update.c
    Core/Src/common/ota_metadata.c
    Core/Src/common/uart_debug.c
    Core/Src/boot/bootloader.c
    Core/Src/boot/bootloader_main.c
    Core/Src/boot/bootloader_it.c
    Core/Src/common/syscalls.c
    Core/Src/common/sysmem.c
    Core/Src/common/led.c
)

target_link_libraries(OTA_bootloader
    stm32cubemx
    STM32_Drivers
    m
)

target_link_options(OTA_bootloader PRIVATE
    -T "${CMAKE_SOURCE_DIR}/STM32F072_BOOTLOADER.ld"
    -Wl,-Map=OTA_bootloader.map
    -Wl,--print-memory-usage
)

target_compile_definitions(OTA_bootloader PRIVATE
    FIRMWARE_VERSION=1
)

set_target_properties(OTA_bootloader PROPERTIES
    LINK_DEPENDS "${CMAKE_SOURCE_DIR}/STM32F072_BOOTLOADER.ld"
    ADDITIONAL_CLEAN_FILES "OTA_bootloader.map;OTA_bootloader.bin;OTA_bootloader.hex"
)

# ---------------------------------------------------------------------------
# Additional build steps to generate .hex and .bin files for the applications
# ---------------------------------------------------------------------------
add_custom_command(TARGET OTA_app_a POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex
    $<TARGET_FILE:OTA_app_a>
    ${CMAKE_CURRENT_BINARY_DIR}/OTA_app_a.hex
    COMMENT "Generating OTA_app_a.hex"
)
add_custom_command(TARGET OTA_app_b POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex
    $<TARGET_FILE:OTA_app_b>
    ${CMAKE_CURRENT_BINARY_DIR}/OTA_app_b.hex
    COMMENT "Generating OTA_app_b.hex"
)


add_custom_command(TARGET OTA_app_a POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary
    $<TARGET_FILE:OTA_app_a>
    ${CMAKE_CURRENT_BINARY_DIR}/OTA_app_a.bin
    COMMENT "Generating OTA_app_a.bin"
)
add_custom_command(TARGET OTA_app_b POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary
    $<TARGET_FILE:OTA_app_b>
    ${CMAKE_CURRENT_BINARY_DIR}/OTA_app_b.bin
    COMMENT "Generating OTA_app_b.bin"
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(flash_stm32)
flash_target(OTA_bootloader)
flash_target(OTA_app_a)
flash_target(OTA_app_b)
erase_flash_target()