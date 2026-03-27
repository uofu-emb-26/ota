#include "main.h"
#include "stm32f072xb.h"
#include "stm32f0xx_hal.h"
#include <stdint.h>
#include "flash_update.h"

void flash_unlock(){
    while ((FLASH->SR & FLASH_CR_LOCK) != 0){}

    FLASH->KEYR = FLASH_KEY1; //Check stm32f072xb.h for exact value
    FLASH->KEYR = FLASH_KEY2;
}

void flash_lock(){
    FLASH->CR |= FLASH_CR_LOCK;
}

void flash_write(uint32_t flash_addr, uint16_t data){

    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint16_t *)flash_addr = data;

    while((FLASH->SR & FLASH_SR_BSY) != 0){}

    if ((FLASH->SR & FLASH_SR_EOP) != 0){
        FLASH->SR = FLASH_SR_EOP;
    } else{
            //error handling
    }

    FLASH->CR &= ~FLASH_CR_PG;
}



