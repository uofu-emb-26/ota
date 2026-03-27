#include "main.h"
#include "stm32f072xb.h"
#include "stm32f0xx_hal.h"
#include <stdint.h>
#include "flash_update.h"

/* The STM32F0xx embedded flash memory can be programmed using in-circuit
programming or in-application programming.

In-application programming (IAP) can use any
communication interface supported by the microcontroller (I/Os, USB, CAN, USART, I2C,
SPI, etc.) to download programming data into memory. 

IAP allows the user to re-program
the flash memory while the application is running. Nevertheless, part of the application has
to have been previously programmed in the flash memory using ICP.

(pg. 59 in 936 datasheet)
*/
void flash_unlock(){
    while ((FLASH->SR & FLASH_CR_LOCK) != 0){}

    /* An unlocking sequence should be written to the FLASH_KEYR
       register to open the access to the FLASH_CR register. This sequence consists of two write
       operations:
            • Write KEY1 = 0x45670123
            • Write KEY2 = 0xCDEF89AB */

    FLASH->KEYR = FLASH_KEY1; 
    FLASH->KEYR = FLASH_KEY2;
}

void flash_lock(){
    FLASH->CR |= FLASH_CR_LOCK;
}

/**
 * @brief 
 The main flash memory programming sequence in standard mode is as follows:
1. Check that no main flash memory operation is ongoing by checking the BSY bit in the
    FLASH_SR register.
2. Set the PG bit in the FLASH_CR register.
3. Perform the data write (half-word) at the desired address.
4. Wait until the BSY bit is reset in the FLASH_SR register.
5. Check the EOP flag in the FLASH_SR register (it is set when the programming
operation has succeeded), and then clear it by software.
The registers are not accessible in write mode when the BSY bit of the FLASH_SR register
is set.
 * 
 * @param flash_addr 
 * @param data 
 */
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

void flash_page_remove(uint32_t del_addr){
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR |= del_addr; 
    FLASH->CR |= FLASH_CR_STRT;

    while((FLASH->SR & FLASH_SR_BSY) != 0){}

    if((FLASH->SR & FLASH_SR_EOP) != 0){
        FLASH->SR = FLASH_SR_EOP;
    } else {
        //error handling
    }

    FLASH->CR &= ~FLASH_CR_PER;
}



