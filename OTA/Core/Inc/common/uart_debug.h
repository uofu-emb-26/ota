#ifndef UART_DEBUG_H
#define UART_DEBUG_H

/**
 * @brief  Initialise USART3 TX for debug output.
 *         Pin: PC10 = USART3_TX (AF1), 115200 baud, 8N1, TX-only, polling.
 *         Call once after SystemClock_Config() (or bl_clock_init() in bootloader).
 */
void uart_debug_init(void);

/**
 * @brief  Transmit a null-terminated string over USART3 (blocking).
 */
void uart_debug_transmit(const char *str);

#endif /* UART_DEBUG_H */
