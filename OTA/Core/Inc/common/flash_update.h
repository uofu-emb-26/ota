#ifndef FLASH_UPDATE_H
#define FLASH_UPDATE_H

#include <stdint.h>
#include <stm32f072xb.h>

#define FLASH_PAGE_SIZE_BYTES 2048U

/* Unlock / lock the flash programming interface */
void flash_unlock(void);
void flash_lock(void);

/* Erase a single 2 KB page.
 * page_addr must be any address within the target page (page-aligned).
 * Returns 0 on success, non-zero on failure. */
int flash_erase_page(uint32_t page_addr);

/* Program a single 16-bit half-word.
 * Flash must be unlocked before calling.
 * Returns 0 on success, non-zero on failure. */
int flash_write(uint32_t flash_addr, uint16_t data);

/* Unlock, write len bytes from buf to dest_addr, then lock.
 * dest_addr must be half-word aligned; len is rounded up to next even byte.
 * Returns 0 on success, non-zero on first failure. */
int flash_write_buf(uint32_t dest_addr, const uint8_t *buf, uint32_t len);

/* Legacy alias kept for existing call sites */
static inline int flash_page_remove(uint32_t addr)
{
	flash_unlock();
	int r = flash_erase_page(addr);
	flash_lock();
	return r;
}

/* Unlock, erase page_total consecutive 2 KB pages starting at start_addr,
 * then lock flash.
 * start_addr should be page-aligned.
 * Returns 0 on success, non-zero on first erase failure. */
int flash_clr_upd_region(uint32_t start_addr, uint32_t page_total);

/* Receive fixed 3-byte UART records and write type-1 records as 16-bit
 * half-words into the inactive application slot.
 * Record format: [type][first_byte][second_byte].
 * Type 0 ends transfer; type 1 writes data; type 2 is reserved.
 * Returns 0 on successful end record, non-zero on failure. */
int flash_write_from_uart(USART_TypeDef *uart, uint32_t page_total);

/* Blocking transmit of one byte over the selected USART.
 * Waits until TDR is empty before writing. */
void transmit_char(uint8_t out, USART_TypeDef *uart);

/* Blocking receive of one byte over the selected USART.
 * Waits until RDR contains unread data before returning. */
uint8_t receive_char(USART_TypeDef *uart);

void button_interrupt_config(void);

#endif /* FLASH_UPDATE_H */

