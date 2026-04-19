#include "stm32f0xx_hal.h"
#include "bootloader.h"
#include "ota_config.h"
#include "uart_debug.h"
#include "led.h"

/* Simple RCC init for the bootloader – run at default 8 MHz HSI.
 * No PLL, no peripherals.  HAL_Init sets up SysTick at 1 kHz which
 * is required by HAL_FLASH_* timeout machinery. */
static void bl_clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState       = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState   = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType    = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0);
}

int main(void)
{
    HAL_Init();
    bl_clock_init();

    uart_debug_init();
    uart_debug_transmit("**** RESET, in Bootloader ****\r\n");
    led_init();
    led_on();
    HAL_Delay(150U); //use of blocking delay is acceptable here since the bootloader does not have real-time constraints
    led_off();
    /* Run boot decision engine.  On success this never returns. */
    boot_result_t result = bootloader_run();

    /* Only reached if no valid slot found – signal with a busy-loop.
     * Future extension: assert a GPIO error LED or wait for transport. */
    (void)result;
    while (1) 
    {
      led_alternate(150U);
    }
}
