#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>
#include <stm32f0xx_hal.h>
#include <stm32f0xx_hal_gpio.h>

/* GPIO Configuration and Control Functions */
void GPIO_Init(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t mode, uint32_t speed,
               uint32_t pull);
void GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t pin);
void GPIO_EnableClock(GPIO_TypeDef *GPIOx);
uint32_t GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint32_t pin);
void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t state);
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint32_t pin);
void GPIO_SetAlternateFunction(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t af);

/* EXTI Configuration Functions */
void EXTI_Config(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t rising_edge,
                 uint32_t falling_edge);
void EXTI_EnableInterrupt(uint32_t pin, uint32_t priority);
void EXTI_ClearPending(uint32_t pin);

/* I2C Configuration Functions */
void I2C2_RCC_CLK_Enable(void);
void I2C2_Init(void);
int I2C_WriteReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
int I2C_ReadReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
#endif /* HAL_GPIO_H */