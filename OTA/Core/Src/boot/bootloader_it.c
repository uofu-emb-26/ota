#include "stm32f0xx_hal.h"

/* --------------------------------------------------------------------------
 * Minimal bootloader ISR overrides
 *
 * HAL_Init enables SysTick at 1 ms. Without this handler, the weak alias in
 * startup_stm32f072xb.s routes SysTick to Default_Handler (infinite loop).
 * --------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
    HAL_IncTick();
}
