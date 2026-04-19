#include "hal_gpio.h"

void GPIO_Init(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t mode, uint32_t speed,
               uint32_t pull) {
  uint32_t position = pin * 2; // Position for 2-bit fields

  // Extract the actual mode and output type from the combined mode parameter
  uint32_t gpio_mode = (mode & GPIO_MODE) >> GPIO_MODE_Pos;       // Bits 0-1
  uint32_t output_type = (mode & OUTPUT_TYPE) >> OUTPUT_TYPE_Pos; // Bit 4

  // Configure Mode, 2 bits per pin
  GPIOx->MODER &= ~(0x3UL << position);
  GPIOx->MODER |= (gpio_mode << position);

  // Configure Output Type (only relevant for output modes)
  if (gpio_mode == GPIO_MODE_OUTPUT_PP || gpio_mode == GPIO_MODE_AF_PP ||
      gpio_mode == GPIO_MODE_OUTPUT_OD || gpio_mode == GPIO_MODE_AF_OD) {
    GPIOx->OTYPER &= ~(0x1UL << pin);
    GPIOx->OTYPER |= (output_type << pin);
  }

  // Configure Speed, 2 bits per pin
  GPIOx->OSPEEDR &= ~(0x3UL << position);
  GPIOx->OSPEEDR |= (speed << position);

  // Configure pull-up/pull-down, 2 bits per pin
  GPIOx->PUPDR &= ~(0x3UL << position);
  GPIOx->PUPDR |= (pull << position);
}

void GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t pin) {
  // Reset logic can be implemented here if needed
  // Typically involves resetting registers to default values
}

uint32_t GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint32_t pin) {
  if (GPIOx->IDR & (0x1UL << pin)) {
    return GPIO_PIN_SET;
  }
  return GPIO_PIN_RESET;
}

void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t state) {
  if (state == GPIO_PIN_SET) {
    GPIOx->BSRR = (0x1UL << pin); // Lower 16 bits to SET
  } else {
    GPIOx->BSRR = (0x1UL << (pin + 16)); // Upper 16 bits to RESET
  }
}

/**
 * @brief
 */
void GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint32_t pin) {
  GPIOx->ODR ^= (0x1UL << pin);
}

/**
 * @brief Helper to calculate port index (A=0, B=1, etc.) based on memory
 * address. Assumes ports are spaced by 0x400 bytes, which is standard for
 * STM32F0
 */
static inline uint32_t GPIO_GetPortIndex(GPIO_TypeDef *port) {
  return ((uint32_t)port - (uint32_t)GPIOA) / 0x400;
}

/**
 * @brief Enable GPIO clock in RCC
 */
void GPIO_EnableClock(GPIO_TypeDef *GPIOx) {
  // On STM32F0, GPIO clock enable bits in AHBENR start at bit 17 (GPIOA)
  // and are contiguous for A, B, C, D, E, F.
  // GPIOA_EN = Bit 17
  // GPIOB_EN = Bit 18
  // ...
  uint32_t port_idx = GPIO_GetPortIndex(GPIOx);

  // 17 is the bit position for GPIOAEN in RCC->AHBENR
  RCC->AHBENR |= (1UL << (17 + port_idx));
}

/**
 * @brief Set alternate function for a pin
 */
void GPIO_SetAlternateFunction(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t af) {
  // AFR[0] = AFRL for pins 0-7
  // AFR[1] = AFRH for pins 8-15
  // Each pin uses 4 bits

  uint32_t regIdx = pin / 8;       // 0 for pins 0-7, 1 for pins 8-15
  uint32_t bitPos = (pin % 8) * 4; // Position within the register

  GPIOx->AFR[regIdx] &= ~(0xF << bitPos); // Clear the 4 AF bits
  GPIOx->AFR[regIdx] |= (af << bitPos);   // Set the AF number
}

/**
 * @brief Configure EXTI for external interrupt on a pin
 */
void EXTI_Config(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t rising_edge,
                 uint32_t falling_edge) {
  // Determine port number
  uint32_t port_index = GPIO_GetPortIndex(GPIOx);

  // Enable SYSCFG clock
  __HAL_RCC_SYSCFG_CLK_ENABLE();

  // Configure SYSCFG EXTICR to route pin to EXTI
  // EXTICR[0] = pins 0-3, EXTICR[1] = pins 4-7, etc.
  uint32_t register_index = pin / 4;
  uint32_t bit_position = (pin % 4) * 4;

  SYSCFG->EXTICR[register_index] &= ~(0xFUL << bit_position);     // Clear
  SYSCFG->EXTICR[register_index] |= (port_index << bit_position); // Set port

  // Configure trigger edges
  if (rising_edge)
    EXTI->RTSR |= (1UL << pin);
  else
    EXTI->RTSR &= ~(1UL << pin);

  if (falling_edge)
    EXTI->FTSR |= (1UL << pin);
  else
    EXTI->FTSR &= ~(1UL << pin);

  // Unmask/enable interrupt on this line
  EXTI->IMR |= (1UL << pin);
}

/**
 * @brief Enable EXTI interrupt in NVIC
 */
void EXTI_EnableInterrupt(uint32_t pin, uint32_t priority) {
  IRQn_Type irq_number;

  // Map EXTI line to NVIC IRQ
  if (pin <= 1)
    irq_number = EXTI0_1_IRQn;
  else if (pin <= 3)
    irq_number = EXTI2_3_IRQn;
  else
    irq_number = EXTI4_15_IRQn;

  NVIC_SetPriority(irq_number, priority);
  NVIC_EnableIRQ(irq_number);
}

/**
 * @brief Clear EXTI pending flag (call this in your ISR!)
 */
void EXTI_ClearPending(uint32_t pin) {
  EXTI->PR = (1UL << pin); // Write 1 to clear
}

/**
 * @brief Enable the I2C2 clock in RCC
 */
void I2C2_RCC_CLK_Enable(void) {
  RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
}

/**
 * @brief Initialize I2C2 peripheral
 */
void I2C2_Init(void) {
  // Disable peripheral first to allow configuration
  I2C2->CR1 &= ~I2C_CR1_PE;

  // Set TIMINGR for 100 kHz
  I2C2->TIMINGR = (1UL << 28) |  // PRESC
                  (0x4UL << 20) | // SCLDEL
                  (0x2UL << 16) | // SDADEL
                  (0xFUL << 8) |  // SCLH
                  (0x13UL << 0);  // SCLL

  // Enable the peripheral
  I2C2->CR1 |= I2C_CR1_PE;
}

/**
 * @brief Write one or more bytes to a slave register
 * @param slaveAddr  7-bit slave address
 * @param regAddr    Register address to write to
 * @param data       Pointer to data bytes
 * @param len        Number of bytes to write
 * @return 0 on success, -1 on NACK
 */
int I2C_WriteReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
  // Set up CR2: slave address, NBYTES = len+1 (reg addr + data), write transfer
  // Clear NBYTES, SADD, RD_WRN
  I2C2->CR2 &= ~((0xFFUL << 16) | (0x3FFUL << 0) | I2C_CR2_RD_WRN);
  // Set NBYTES and SADD
  I2C2->CR2 |= (((uint32_t)len + 1) << 16) | ((uint32_t)slaveAddr << 1);

  // Send START
  I2C2->CR2 |= I2C_CR2_START;

  // Wait for TXIS (Transmit Interrupt Status) or NACKF (NACK Flag)
  while (!(I2C2->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)))
    ;
  if (I2C2->ISR & I2C_ISR_NACKF) {
    I2C2->ICR |= I2C_ICR_NACKCF; // Clear NACK flag
    return -1;
  }

  // Write register address
  I2C2->TXDR = regAddr;

  // Write each data byte
  for (uint8_t i = 0; i < len; i++) {
    // Wait for TXIS or NACKF
    while (!(I2C2->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)))
      ;
    
    if (I2C2->ISR & I2C_ISR_NACKF) {
      I2C2->ICR |= I2C_ICR_NACKCF;
      return -1;
    }
    I2C2->TXDR = data[i];
  }

  // Wait for TC (Transfer Complete)
  while (!(I2C2->ISR & I2C_ISR_TC))
    ;

  // Send STOP
  I2C2->CR2 |= I2C_CR2_STOP;

  // Wait for STOPF (Stop Flag)
  while (!(I2C2->ISR & I2C_ISR_STOPF))
    ;
  
  I2C2->ICR |= I2C_ICR_STOPCF; // Clear STOP flag

  return 0;
}

/**
 * @brief Read one or more bytes from a slave register
 * @param slaveAddr  7-bit slave address
 * @param regAddr    Register address to read from
 * @param data       Pointer to buffer for read data
 * @param len        Number of bytes to read
 * @return 0 on success, -1 on NACK
 */
int I2C_ReadReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
  // Write the register address
  I2C2->CR2 &= ~((0xFFUL << 16) | (0x3FFUL << 0) | I2C_CR2_RD_WRN);
  I2C2->CR2 |= (1UL << 16) | ((uint32_t)slaveAddr << 1); // NBYTES=1, write
  I2C2->CR2 |= I2C_CR2_START;

  while (!(I2C2->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)))
    ;
  if (I2C2->ISR & I2C_ISR_NACKF) {
    I2C2->ICR |= I2C_ICR_NACKCF;
    return -1;
  }

  I2C2->TXDR = regAddr;

  while (!(I2C2->ISR & I2C_ISR_TC))
    ;

  // Restart and read
  I2C2->CR2 &= ~((0xFFUL << 16) | (0x3FFUL << 0));
  I2C2->CR2 |= ((uint32_t)len << 16) | ((uint32_t)slaveAddr << 1) | I2C_CR2_RD_WRN;
  I2C2->CR2 |= I2C_CR2_START; // Restart condition

  for (uint8_t i = 0; i < len; i++) {
    while (!(I2C2->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF)))
      ;
    if (I2C2->ISR & I2C_ISR_NACKF) {
      I2C2->ICR |= I2C_ICR_NACKCF;
      return -1;
    }
    data[i] = (uint8_t)I2C2->RXDR;
  }

  while (!(I2C2->ISR & I2C_ISR_TC))
    ;

  // Send STOP
  I2C2->CR2 |= I2C_CR2_STOP;

  // Wait for stop
  while (!(I2C2->ISR & I2C_ISR_STOPF))
    ;
  I2C2->ICR |= I2C_ICR_STOPCF; // clear the flag

  return 0;
}
