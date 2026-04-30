
#include <uart3.h>
#include "main.h"
#include <stdint.h>

/* Enable the system clock to the desired USART in the RCC peripheral. */
void Enable_SysClock_USART()
{
  RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
}

/* Set the Baud rate for communication to be 115200 bits/second. 
Using the HAL_RCC_GetHCLKFreq() function to get the system clock frequency.
*/
void Set_Baud_Rate()
{
  /* Set the Baud Rate to 115200 bits/second 
  Baud_Divider = 8000000/115200 = 69.444
  USART_BRR = 69*/
  uint32_t sys_clock_freq = HAL_RCC_GetHCLKFreq();
  uint32_t target_baud = 115200;
  USART3->BRR = sys_clock_freq / target_baud;
}

/* Set the selected pins into alternate function mode and program the correct alternate function
number into the GPIO AFR registers.
PC4 - TX - green jumper
PC5 - RX - red jumper
*/
void Config_Pins()
{
  GPIOC->MODER |= GPIO_MODER_MODER4_1;
  GPIOC->MODER |= GPIO_MODER_MODER5_1;
  GPIOC->MODER &= ~(GPIO_MODER_MODER4_0);
  GPIOC->MODER &= ~(GPIO_MODER_MODER5_0);

  GPIOC->AFR[0] |= 1 << (4*4);
  GPIOC->AFR[0] |= 1 << (4*5);
}

/* Enable the CR1 in USART3*/
void Enable_USART_Control()
{
  USART3->CR1 |= USART_CR1_UE;
}

/* The transmitter can send data words of either 7, 8 or 9 bits depending on the M bits status.
The Transmit Enable bit (TE) must be set in order to activate the transmitter function. The
data in the transmit shift register is output on the TX pin and the corresponding clock pulses
are output on the CK pin. 

See details of the USART RX on DM---936, section 27.5.3
*/
void Enable_TX_RX()
{
  USART3->CR1 |= USART_CR1_TE;
  USART3->CR1 |= USART_CR1_RE;
}

void Enable_Receive_Register_NE()
{
  USART3->CR1 |= USART_CR1_RXNEIE;
}