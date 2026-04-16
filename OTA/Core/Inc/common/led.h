#ifndef LED_H
#define LED_H

#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include <stdint.h>
#include <stdio.h>

void led_init(void);
void led_clockwise(uint32_t speed_ms);
void led_alternate(uint32_t speed_ms);
void led_counterclockwise(uint32_t speed_ms);
void led_off(void);
void led_on(void);

#endif /* LED_H */