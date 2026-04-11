/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_gcc.h"
#include "stm32f072xb.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "flash_update.h"
#include "stm32f0xx_hal_rcc.h"
#include "ota_metadata.h"
#include <stdint.h>
#include <sys/_intsup.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_RED_PIN     GPIO_PIN_6
#define LED_BLUE_PIN    GPIO_PIN_7
#define LED_ORANGE_PIN  GPIO_PIN_8
#define LED_GREEN_PIN   GPIO_PIN_9

/* USER CODE END PD */
// enable test code using macros

#define APP_UART_ENABLE     0U
#define APP_UART3_ENABLE    1U
#define COLOR_UART          0U
#define PROGRAM_STORE_UART  1U
/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;
I2C_HandleTypeDef hi2c2;
SPI_HandleTypeDef hspi2;
TSC_HandleTypeDef htsc;

#if (APP_UART3_ENABLE)
#if (COLOR_UART)
volatile uint8_t cmd_arr[] = {0x11,0x22};
#endif
#if (PROGRAM_STORE_UART)
uint8_t cmd_arr[400];
uint8_t cmd_arr_pointer = 0;
#endif
uint8_t rr_val;
uint8_t new_data;
uint8_t new_data_h;
uint8_t invalid_cmd;
#endif



#if (APP_UART_ENABLE == 1U)
  UART_HandleTypeDef huart4;
  DMA_HandleTypeDef hdma_usart4_rx;
  DMA_HandleTypeDef hdma_usart4_tx;

  uint8_t recieved_data[400];
  static uint16_t uart_size;
  char application_message[] = "Hello from application 1!";
#endif /* APP_UART_ENABLE */

PCD_HandleTypeDef hpcd_USB_FS;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI2_Init(void);
static void MX_TSC_Init(void);
static void MX_USB_PCD_Init(void);
static void MX_CRC_Init(void);
#if (APP_UART_ENABLE == 1U)
  static void MX_USART4_UART_Init(void);
#endif


/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#if (APP_UART3_ENABLE)
void Reset_Cmd_Arr(){
  cmd_arr[0] = 0x11;
  cmd_arr[1] = 0x22;
}


/**
Red: PC6 of the STM32F072RBT6.
Orange PC8 of the STM32F072RBT6.
Green  PC9 of the STM32F072RBT6.
Blue  PC7 of the STM32F072RBT6.
 */
void My_HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->ODR ^= GPIO_Pin;
}
#endif

#if (APP_UART_ENABLE == 1U)
// Number of data available in application reception buffer (indicates a position in reception buffer until which, data are available)
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  uart_size = Size;
  //huart -> UART handle, recieved_data -> pointer to buffer, sizeof(recieved_data) -> amount of data to recieve
  HAL_UARTEx_ReceiveToIdle_DMA(&huart4, recieved_data, sizeof(recieved_data));
  
}
#endif /* APP_UART_ENABLE */


#if (APP_UART3_ENABLE)
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
PC4 - TX
PC5 - RX
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

// void Enable_NVIC()
// {
//   NVIC_EnableIRQ(USART3_4_IRQn);
//   NVIC_SetPriority(USART3_4_IRQn, 1);
// }

/*
Setup the blank interrupt handler.
Within the handler, save the receive register’s value into a global variable.
Within the handler set a global variable as a flag indicating new data.
*/
// void USART3_4_IRQHandler()
// {
//   rr_val = USART3->RDR;
//   new_data_h = 1;
// }

#if (COLOR_UART)
int Match_CMD_To_LED(uint8_t recieved_data, int num_of_cmds)
{
  int flag = 1;
  if (recieved_data == 0x72)
  {
    flag = 0;
    My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
    return flag;
  } 
  else if (recieved_data == 0x62)
  {
    flag = 0;
    My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
    return flag;
  }
  else if (recieved_data == 0x6F)
  {
    flag = 0;
    My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
    return flag;
  }
  else if (recieved_data == 0x67)
  {
    flag = 0;
    My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
    return flag;
  }
  else
  {
    // char *error_string = "Invalid Keystroke";
    // Transmit_String(error_string, num_of_cmds);
    return flag;    
  }

}



/**
Red: PC6 of the STM32F072RBT6.
Orange PC8 of the STM32F072RBT6.
Green  PC9 of the STM32F072RBT6.
Blue  PC7 of the STM32F072RBT6.
 */
void Execute_Cmd()
{
  //red LED commands
  if (cmd_arr[0] == 0x72) 
  {
    if(cmd_arr[1] == 0x30)
    {
      GPIOC->ODR &= ~(GPIO_ODR_6);
    }
    else if (cmd_arr[1] == 0x31) {
      GPIOC->ODR |= (GPIO_ODR_6);
    }
    else if (cmd_arr[1] == 0x32) {
      My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
    }
  }

  //blue LED commands
  if (cmd_arr[0] == 0x62) 
  {
    if(cmd_arr[1] == 0x30)
    {
      GPIOC->ODR &= ~(GPIO_ODR_7);
    }
    else if (cmd_arr[1] == 0x31) {
      GPIOC->ODR |= (GPIO_ODR_7);
    }
    else if (cmd_arr[1] == 0x32) {
      My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
    }
  }

  //green LED commands
  if (cmd_arr[0] == 0x67) 
  {
    if(cmd_arr[1] == 0x30)
    {
      GPIOC->ODR &= ~(GPIO_ODR_9);
    }
    else if (cmd_arr[1] == 0x31) {
      GPIOC->ODR |= (GPIO_ODR_9);
    }
    else if (cmd_arr[1] == 0x32) {
      My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
    }
  }

  //orange LED commands
  if (cmd_arr[0] == 0x6F) 
  {
    if(cmd_arr[1] == 0x30)
    {
      GPIOC->ODR &= ~(GPIO_ODR_8);
    }
    else if (cmd_arr[1] == 0x31) {
      GPIOC->ODR |= (GPIO_ODR_8);
    }
    else if (cmd_arr[1] == 0x32) {
      My_HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
    }
  }
  Reset_Cmd_Arr();
}

int Valid_LED_Cmd(uint8_t data)
{
  if(data == 0x72 || data == 0x62 || data == 0x6F || data == 0x67)
  {
    return 1;
  }
  return 0;
}

int Valid_Num_Cmd(uint8_t data)
{
  if(data == 0x30|| data == 0x31 || data == 0x32)
  {
    return 1;
  }
  return 0;
}
#endif

void Check_Data(int num_of_cmds){
  /* 1. Check and wait on the USART status flag that indicates 
      the receive (read) register is not empty
     2. Use an empty while loop which exits once the flag is set
   */
  int flag = 1;

  while(flag){
    if(USART3->ISR & USART_ISR_RXNE_Msk){
      if (num_of_cmds == 1)
      {
        #if COLOR_UART
        uint8_t recieved_data = USART3->RDR;
        flag = Match_CMD_To_LED(recieved_data, num_of_cmds);
        #endif
      }
      else 
      {
        uint8_t recieved_data = USART3->RDR;
        
        #if (COLOR_UART)
        if (Valid_LED_Cmd(recieved_data)) {
          cmd_arr[0] = recieved_data;
          flag = 0;
        }
        else if (Valid_Num_Cmd(recieved_data)) {
          cmd_arr[1] = recieved_data;
          new_data = 1;
          flag = 0;
        }
        else {
          // char *error_string = "Invalid Keystroke";
          // Transmit_String(error_string, num_of_cmds);
          Match_CMD_To_LED('d', 1);
          flag = 0;
        }
        #endif

        #if (PROGRAM_STORE_UART)
        cmd_arr[cmd_arr_pointer] = recieved_data;
        cmd_arr_pointer++;
        flag = 0;
        #endif
      }
    }
  }
}

void Parse_Program()
{
  HAL_GPIO_TogglePin(GPIOC, LED_GREEN_PIN);

  // char* cmd_arr_string = "";
  // char * check_string = "hello world";
  // for (int i = 0; i < 11; i++) {
  //   cmd_arr_string[i] = cmd_arr[i];
  // }
  // if (cmd_arr_string == check_string) {
  //   HAL_GPIO_TogglePin(GPIOC, LED_GREEN_PIN);
  // }
}
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C2_Init();
  MX_SPI2_Init();
  MX_TSC_Init();
  MX_USB_PCD_Init();
  MX_CRC_Init();
  #if (APP_UART_ENABLE == 1U)
  MX_USART4_UART_Init();
  #endif /* APP_UART_ENABLE */
  /* USER CODE BEGIN 2 */

  #if (APP_UART3_ENABLE)
  Enable_SysClock_USART();
  Set_Baud_Rate();
  Config_Pins();
  Enable_TX_RX();
  Enable_USART_Control();
  #endif

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitTypeDef initStr1 = {
      GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9, // LEDs
      GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW};

  HAL_GPIO_Init(GPIOC, &initStr1);

  /*GPIO_InitTypeDef initStr2 = {GPIO_PIN_0, //Pushbutton
                              GPIO_MODE_INPUT,
                              GPIO_PULLDOWN,
                              GPIO_SPEED_FREQ_LOW};

                              HAL_GPIO_Init(GPIOA,&initStr2);
*/
  //button_interrupt_config();
  __NVIC_EnableIRQ(EXTI0_1_IRQn);
  NVIC_SetPriority(EXTI0_1_IRQn,1);

  /* Confirm this slot is healthy – clears trial-boot counter in metadata.
   * Move this call later (after connectivity/sensor checks) for a real app. */
  ota_confirm_current_slot();


  #if (APP_UART_ENABLE == 1U)
  HAL_UARTEx_ReceiveToIdle_DMA(&huart4, recieved_data, sizeof(recieved_data));
  #endif /* APP_UART_ENABLE */

  /* USER CODE END 2 */
  //HAL_GPIO_WritePin(GPIOC, LED_BLUE_PIN, GPIO_PIN_SET);
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    #if (APP_UART_ENABLE == 1U)
    if(uart_size)
     {
      // /* returns 1 if the CRC doesn't pass */
      // int crc_flag = boot_verify_crc(recieved_data, uart_size - 4, *((uint32_t *)&recieved_data[uart_size - 4]));

      // if(crc_flag == 1)
      // {
      //   //doesn't match
      //   HAL_GPIO_TogglePin(GPIOC, LED_RED_PIN);
      // }
      // else
      // {
      //   //match
      //   HAL_GPIO_TogglePin(GPIOC, LED_GREEN_PIN);
      //   // printf("message: % \n", (char*)recieved_data);
      // }
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
     }
#endif /* APP_UART_ENABLE */

    // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
    // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
    // for (volatile uint32_t i = 0; i < 10000000; i++) 
    // {
    //   __NOP();
    // }
    #if (APP_UART3_ENABLE)
    //USART3 stuff:
    #if (COLOR_UART)
    int number_of_cmds = 2;
    Check_Data(number_of_cmds);
    if (cmd_arr[1] == 0x22) {
     continue; 
    }

    if (number_of_cmds == 2 && new_data) 
    {
      Execute_Cmd();
    }
    #endif
    #if (PROGRAM_STORE_UART)
    Check_Data(2);
    if (cmd_arr_pointer > 11)
    {
      Parse_Program();  
    }
    #endif
    // --------end of usart3 stuff--------
    #endif


    //HAL_Delay(100000);
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

void EXTI0_1_IRQHandler(void){
  flash_unlock();
  flash_erase_page(0x08010000);
  flash_write(0x08010000, 0x0001);
  flash_lock();
  EXTI->PR = (1);

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x20303E5D;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TSC Initialization Function
  * @param None
  * @retval None
  */
static void MX_TSC_Init(void)
{

  /* USER CODE BEGIN TSC_Init 0 */

  /* USER CODE END TSC_Init 0 */

  /* USER CODE BEGIN TSC_Init 1 */

  /* USER CODE END TSC_Init 1 */

  /** Configure the TSC peripheral
  */
  htsc.Instance = TSC;
  htsc.Init.CTPulseHighLength = TSC_CTPH_2CYCLES;
  htsc.Init.CTPulseLowLength = TSC_CTPL_2CYCLES;
  htsc.Init.SpreadSpectrum = DISABLE;
  htsc.Init.SpreadSpectrumDeviation = 1;
  htsc.Init.SpreadSpectrumPrescaler = TSC_SS_PRESC_DIV1;
  htsc.Init.PulseGeneratorPrescaler = TSC_PG_PRESC_DIV4;
  htsc.Init.MaxCountValue = TSC_MCV_8191;
  htsc.Init.IODefaultMode = TSC_IODEF_OUT_PP_LOW;
  htsc.Init.SynchroPinPolarity = TSC_SYNC_POLARITY_FALLING;
  htsc.Init.AcquisitionMode = TSC_ACQ_MODE_NORMAL;
  htsc.Init.MaxCountInterrupt = DISABLE;
  htsc.Init.ChannelIOs = TSC_GROUP1_IO3|TSC_GROUP2_IO3|TSC_GROUP3_IO2;
  htsc.Init.ShieldIOs = 0;
  htsc.Init.SamplingIOs = TSC_GROUP1_IO4|TSC_GROUP2_IO4|TSC_GROUP3_IO3;
  if (HAL_TSC_Init(&htsc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TSC_Init 2 */

  /* USER CODE END TSC_Init 2 */

}

#if (APP_UART_ENABLE == 1U)
/**
  * @brief USART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART4_UART_Init(void)
{

  /* USER CODE BEGIN USART4_Init 0 */

  /* USER CODE END USART4_Init 0 */

  /* USER CODE BEGIN USART4_Init 1 */

  /* USER CODE END USART4_Init 1 */
  huart4.Instance = USART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  /*
   * PA1 - USART4_RX
   * PC10 - USART4_TX 
   */

  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART4_Init 2 */

  /* USER CODE END USART4_Init 2 */

}
#endif /* APP_UART_ENABLE */
/**
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_5_6_7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, NCS_MEMS_SPI_Pin|EXT_RESET_Pin|LD3_Pin|LD6_Pin
                          |LD4_Pin|LD5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : NCS_MEMS_SPI_Pin EXT_RESET_Pin LD3_Pin LD6_Pin
                           LD4_Pin LD5_Pin */
  GPIO_InitStruct.Pin = NCS_MEMS_SPI_Pin|EXT_RESET_Pin|LD3_Pin|LD6_Pin
                          |LD4_Pin|LD5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : MEMS_INT1_Pin MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT1_Pin|MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */








  


