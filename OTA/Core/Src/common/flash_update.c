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

/* ---------------------------------------------------------------------------
 * button_interrupt_config  (unchanged – EXTI0 on PA0)
 * ---------------------------------------------------------------------------*/
void button_interrupt_config(void)
{
    EXTI->IMR  |= 1UL;
    EXTI->RTSR |= 1UL;
    SYSCFG->EXTICR[0] &= ~0xFU;
}



