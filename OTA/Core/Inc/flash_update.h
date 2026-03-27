#include "stm32f0xx_hal.h"

void flash_unlock(void);

void flash_lock(void);

void flash_write(uint32_t flash_addr, uint16_t data);

