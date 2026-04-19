function(flash_target target)
    find_program(OPENOCD openocd)
    if (OPENOCD)
        add_custom_target(flash_${target} ${OPENOCD} -f interface/stlink.cfg -f target/stm32f0x.cfg -c 'program "$<TARGET_FILE:${target}>" verify reset exit' DEPENDS ${target})
    else()
        find_program(CUBE cube)
        if (CUBE)
            add_custom_target(flash_${target} ${CUBE} programmer -c port=SWD -d "$<TARGET_FILE:${target}>" DEPENDS ${target})
        else()
            message(WARNING "Could not find programmer executable")
        endif()
    endif()
endfunction()

function(erase_flash_target)
    find_program(CUBE cube)
    if (CUBE)
        add_custom_target(erase_flash
            ${CUBE} programmer -c port=SWD -e all
            COMMENT "Mass erasing STM32 internal flash via CubeProgrammer"
        )
    else()
        find_program(OPENOCD openocd)
        if (OPENOCD)
            # Note: 'stm32f1x' is the OpenOCD flash driver name, not the MCU family.
            # STM32F0x uses the same flash controller as STM32F1x, so this driver
            # is intentionally used here (see flash bank declaration in stm32f0x.cfg).
            set(OPENOCD_ERASE_CMD "init\; reset halt\; stm32f1x mass_erase 0\; reset run\; shutdown")
            add_custom_target(erase_flash
                ${OPENOCD}
                -f interface/stlink.cfg
                -f target/stm32f0x.cfg
                -c "${OPENOCD_ERASE_CMD}"
                COMMENT "Mass erasing STM32 internal flash via OpenOCD"
            )
        else()
            message(WARNING "Could not find programmer executable")
        endif()
    endif()
endfunction()
