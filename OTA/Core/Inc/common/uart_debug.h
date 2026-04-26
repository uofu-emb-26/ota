#ifndef UART_DEBUG_H
#define UART_DEBUG_H

#define DEBUG_UART_ENABLE    0U

/**
 * @brief  Initialise USART4 TX for debug output.
 *         Pin: PA0 = USART4_TX (AF4), 115200 baud, 8N1, TX-only, polling.
 *         Call once after SystemClock_Config() (or bl_clock_init() in bootloader).
 */
void uart_debug_init(void);

/**
 * @brief  Transmit a null-terminated string over USART4 (blocking).
 */
void uart_debug_transmit(const char *str);

#endif /* UART_DEBUG_H */
