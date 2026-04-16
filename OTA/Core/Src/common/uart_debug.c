#include "uart_debug.h"
#include "stm32f0xx_hal.h"

/*
 * Minimal USART3 TX driver — direct register access, polling, TX-only.
 * Modelled after the proven implementation in temp/lab4/Src/lab4.c.
 *
 * Hardware wiring (connect external USB-UART bridge / FTDI):
 *   MCU PC10  →  FTDI RX   (USART3_TX, AF1)
 *   MCU GND   →  FTDI GND
 *   115200 8N1, no flow control
 */

void uart_debug_init(void)
{
    /* Enable peripheral clocks */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    /* PC10 = USART3_TX, alternate function 1, push-pull */
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_10;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF1_USART3;
    HAL_GPIO_Init(GPIOC, &g);

    /* Baud-rate: PCLK1 / 115200 (e.g. 48 000 000 / 115200 = 416) */
    USART3->BRR = HAL_RCC_GetPCLK1Freq() / 115200U;

    /* Enable TX + UART; RE is left off — RX not needed for diagnostics */
    USART3->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void uart_debug_transmit(const char *str)
{
    while (*str != '\0') {
        /* Wait for TXE (transmit data register empty) */
        while (!(USART3->ISR & USART_ISR_TXE)) {}
        USART3->TDR = (uint32_t)(*str++);
    }
    /* Wait for TC (transmission complete) so the last byte fully leaves */
    while (!(USART3->ISR & USART_ISR_TC)) {}
}
