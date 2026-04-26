/**
 ******************************************************************************
 * @file           : led.c
 * @brief          : On board LED control functions
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "led.h"
#include "main.h"
#include <stdint.h>

int led_pins[4] = {LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN, LED_ORANGE_PIN};

void led_init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef initStr1 = {
        LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN | LED_ORANGE_PIN, // LEDs
        GPIO_MODE_OUTPUT_PP, // Output, push-pull
        GPIO_NOPULL, // No pull-up or pull-down resistors
        GPIO_SPEED_FREQ_LOW // Low frequency
    };

    HAL_GPIO_Init(GPIOC, &initStr1);
}

void led_clockwise(uint32_t speed_ms)
{
  static uint32_t prev_tick_ms = 0U;
  static uint32_t i = 0U;
  uint32_t curr_tick_ms = HAL_GetTick();

  if ((curr_tick_ms - prev_tick_ms) >= speed_ms)
  {
    led_off();
    HAL_GPIO_WritePin(GPIOC, led_pins[i], GPIO_PIN_SET);
    i = (i + 1U) % 4U; // Move to the next LED in a circular manner
    prev_tick_ms = curr_tick_ms;
  }
}

void led_alternate(uint32_t speed_ms) {
  static uint32_t prev_tick_ms = 0U;
  static uint8_t phase = 0U;
  uint32_t curr_tick_ms = HAL_GetTick();

  if ((curr_tick_ms - prev_tick_ms) < speed_ms) {
    return;
  }

  prev_tick_ms = curr_tick_ms;

  if (phase == 0U) {
    HAL_GPIO_WritePin(GPIOC, LED_RED_PIN | LED_BLUE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, LED_GREEN_PIN | LED_ORANGE_PIN, GPIO_PIN_RESET);
    phase = 1U;
  } else {
    HAL_GPIO_WritePin(GPIOC, LED_RED_PIN | LED_BLUE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LED_GREEN_PIN | LED_ORANGE_PIN, GPIO_PIN_SET);
    phase = 0U;
  }
}
void led_counterclockwise(uint32_t speed_ms) 
{
  static uint32_t prev_tick_ms = 0U;
  static uint32_t i = 2U,j = 0U;
  uint32_t curr_tick_ms = HAL_GetTick();

  if ((curr_tick_ms - prev_tick_ms) >= speed_ms) {
    led_off();
    i = (i + 1U) % 4U; // Move to the next LED in a circular manner
    j = 3U - i; // Calculate the index for counterclockwise direction
    prev_tick_ms = curr_tick_ms;
    HAL_GPIO_WritePin(GPIOC, led_pins[j], GPIO_PIN_SET);
  }
}
void led_off(void)
{
    HAL_GPIO_WritePin(GPIOC, LED_RED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LED_GREEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LED_BLUE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, LED_ORANGE_PIN, GPIO_PIN_RESET);
}

void led_on(void)
{
    HAL_GPIO_WritePin(GPIOC, LED_RED_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, LED_GREEN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, LED_BLUE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, LED_ORANGE_PIN, GPIO_PIN_SET);

}