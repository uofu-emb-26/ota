#include "flash_update.h"
#include "stm32f072xb.h"
#include "stm32f0xx_hal.h"
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * flash_unlock
 *
 * Wait for any in-progress operation to finish, then write the two unlock
 * keys.  The LOCK bit is in FLASH->CR (not FLASH->SR as was incorrect before).
 * ---------------------------------------------------------------------------*/
void flash_unlock(void)
{
    while (FLASH->SR & FLASH_SR_BSY) {}
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

/* ---------------------------------------------------------------------------
 * flash_lock
 * ---------------------------------------------------------------------------*/
void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

/* ---------------------------------------------------------------------------
 * flash_erase_page
 *
 * Erases the 2 KB page that contains page_addr.
 * Flash must already be unlocked.
 * Returns 0 on success, 1 on write-protect or programming error.
 * ---------------------------------------------------------------------------*/
int flash_erase_page(uint32_t page_addr)
{
    /* Wait for any prior operation */
    while (FLASH->SR & FLASH_SR_BSY) {}

    /* Clear error and EOP flags before starting */
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGERR;

    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR  = page_addr;         /* assignment, not OR-assignment */
    FLASH->CR |= FLASH_CR_STRT;

    while (FLASH->SR & FLASH_SR_BSY) {}

    int ok = (FLASH->SR & FLASH_SR_EOP) != 0;
    FLASH->SR  = FLASH_SR_EOP;      /* clear EOP by writing 1 */
    FLASH->CR &= ~FLASH_CR_PER;

    return ok ? 0 : 1;
}

/* ---------------------------------------------------------------------------
 * flash_write
 *
 * Programs a single 16-bit half-word at flash_addr.
 * Flash must already be unlocked.
 * Returns 0 on success, 1 on error.
 * ---------------------------------------------------------------------------*/
int flash_write(uint32_t flash_addr, uint16_t data)
{
    while (FLASH->SR & FLASH_SR_BSY) {}
    FLASH->SR = FLASH_SR_EOP | FLASH_SR_WRPERR | FLASH_SR_PGERR;

    FLASH->CR |= FLASH_CR_PG;
    *(__IO uint16_t *)flash_addr = data;
    while (FLASH->SR & FLASH_SR_BSY) {}

    int ok = (FLASH->SR & FLASH_SR_EOP) != 0;
    FLASH->SR  = FLASH_SR_EOP;
    FLASH->CR &= ~FLASH_CR_PG;

    return ok ? 0 : 1;
}

/* ---------------------------------------------------------------------------
 * flash_write_buf
 *
 * Unlock, program len bytes from buf to dest_addr, then lock.
 * dest_addr must be half-word (2-byte) aligned.
 * If len is odd, a padding 0xFF byte is appended for the final half-word.
 * Returns 0 on success, non-zero on first write failure.
 * ---------------------------------------------------------------------------*/
int flash_write_buf(uint32_t dest_addr, const uint8_t *buf, uint32_t len)
{
    flash_unlock();

    uint32_t i = 0U;
    while (i < len) {
        uint16_t hw;
        uint8_t lo = buf[i];
        uint8_t hi = (i + 1U < len) ? buf[i + 1U] : 0xFFU;
        hw = (uint16_t)lo | ((uint16_t)hi << 8);

        if (flash_write(dest_addr, hw) != 0) {
            flash_lock();
            return 1;
        }
        dest_addr += 2U;
        i         += 2U;
    }

    flash_lock();
    return 0;
}

int flash_clr_upd_region(uint32_t start_addr, uint32_t page_total){
    flash_unlock();
    uint32_t i=0U;
    uint32_t current_addr = start_addr;
    while(i < page_total){
        if(flash_erase_page(start_addr) != 0){
            flash_lock();
            return 1;
        }
        current_addr += 2048;
        i++;
    }

    flash_lock();
    return 0;
}



int flash_write_from_uart(USART_TypeDef *uart, uint32_t page_total){ //assumes fully initialized uart peripheral

    uint8_t current_slot = get_current_slot();
    uint32_t start_address;

    uint8_t type_byte;
    uint8_t first_byte;
    uint8_t second_byte;
    uint16_t write_data;

    uint32_t memory_end;

    if(current_slot == 1){
        start_address = 0x08011800;
    } else if(current_slot == 2){
        start_address = 0x08004000;
    } else {
        transmit_char(0xFF, uart);
        return -1;
    }

    memory_end = start_address + ((page_total) * 2048);

    if (flash_clr_upd_region(start_address) != 0) {
      return -1;
    }
    
    do {

      transmit_char(0, uart); //TODO THIS FUNCTION EXPLICITLY EXCEPTS BEHAVIOR COULD BE ADDED
                              //DICTATING WAIT OR DENY

      type_byte = recieve_char(uart);
      first_byte = recieve_char(uart);
      second_byte = recieve_char(uart);

      write_data = (((uint16_t)first_byte << 8) | ((uint16_t)second_byte));

      if (type_byte == 0) {
        break;
      } else if (type_byte == 1) {

        if ((start_address + 1) >= memory_end) {
          transmit_char(0xFF, uart);
          return -1;
        }

        if (flash_write(start_address, write_data) != 0) {
          transmit_char(0xFF, uart);
          return -1;
        }

        start_address += 2;
        transmit_char(1, uart);

      } else if (type_byte == 2) {
        // placeholder for crc_value handling or other data_type handling
        transmit_char(2, uart);
      } else {
        transmit_char(0xFF, uart);
        return -1;
      }

    } while (1);

    return 0;
}

void transmit_char(uint8_t out, USART_TypeDef *uart){
    while(!(uart->ISR & USART_ISR_TXE)){}
    uart->TDR = out;
}

char receive_char(USART_TypeDef uart){
    while(!(uart->ISR & USART_ISR_RXNE)){}
    return (char)uart->RDR;
}


/* ---------------------------------------------------------------------------
 * button_interrupt_config  (unchanged – EXTI0 on PA0)
 * ---------------------------------------------------------------------------*/
void button_interrupt_config(void)
{
    EXTI->IMR  |= 1UL;
    EXTI->RTSR |= 1UL;
    SYSCFG->EXTICR[0] &= ~0xFU;
}



