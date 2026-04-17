#include "uart_debug.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"

/*
 * Minimal USART4 TX driver — direct register access, polling, TX-only.
 * Modelled after the proven implementation in temp/lab4/Src/lab4.c.
 *
 * Hardware wiring (connect external USB-UART bridge / FTDI):
 *   MCU PA0  →  FTDI RX   (USART4_TX, AF4)
 *   MCU GND   →  FTDI GND
 *   115200 8N1, no flow control
 */

void uart_debug_init(void)
{
    /* Enable peripheral clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART4_CLK_ENABLE();

    /* PCA0 = USART4_TX, alternate function 4, push-pull */
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_0;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_USART4;
    HAL_GPIO_Init(GPIOA, &g);

    /* Baud-rate: PCLK1 / 115200 (e.g. 48 000 000 / 115200 = 416) */
    USART4->BRR = HAL_RCC_GetPCLK1Freq() / 115200U;

    /* Enable TX + UART; RE is left off — RX not needed for diagnostics */
    USART4->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void uart_debug_transmit(const char *str)
{
    while (*str != '\0') {
        /* Wait for TXE (transmit data register empty) */
        while (!(USART4->ISR & USART_ISR_TXE)) {}
        USART4->TDR = (uint32_t)(*str++);
    }
    /* Wait for TC (transmission complete) so the last byte fully leaves */
    while (!(USART4->ISR & USART_ISR_TC)) {}
}
