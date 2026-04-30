

#include <stdint.h>
#include <stm32f072xb.h>

/* Enable the system clock to the desired USART in the RCC peripheral. */
void Enable_SysClock_USART();

//Set the Baud rate for communication to be 115200 bits/second. 
//Using the HAL_RCC_GetHCLKFreq() function to get the system clock frequency.
void Set_Baud_Rate();

//Set the selected pins into alternate function mode and program the correct alternate function
//number into the GPIO AFR registers. PC4 - TX - green jumper, PC5 - RX - red jumper
void Config_Pins();

/* Enable the CR1 in USART3*/
void Enable_USART_Control(void);

void Enable_TX_RX(void);

void Enable_Receive_Register_NE(void);