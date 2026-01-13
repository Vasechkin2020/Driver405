// ver 1.1

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
//----
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "i2c.h"

#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "code.h"
#include "motor.h"
#include "slaveSPI.h"

#include "ina219.h"


void SystemClock_Config(void);
volatile uint32_t millisCounter = 0;

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0); // Принудительно ставим SysTick самый низкий приоритет (15)

  MX_GPIO_Init_Only_Clock(); // Инициализация ТОЛЬКО тактирования GPIO
  MX_USART1_UART_Init();

  HAL_Delay(3000); // Чтобы успеть подключиться терминалом
  printf("\r\n *** Driver ver 1.0 20-12-25 *** printBIM.ru *** 2025 *** \r\n");
  printf("\r\n *** Driver405 printBIM.ru 2025. ***\r\n");

  EnableFPU();    // Включение FPU (CP10 и CP11: полный доступ) Работа с плавающей точкой

  MX_GPIO_Init(); // Инициализация GPIO

  MX_DMA_Init(); // Инициализация DMA

  MX_SPI1_Init(); // Инициализация SPI1
  // HAL_Delay(50000);

  MX_TIM6_Init(); // Инициализация TIM6 для общего цикла
  MX_TIM7_Init(); // Инициализация TIM7 для моторов шаговых ЛЕВЫЙ
  MX_TIM13_Init(); // Инициализация TIM13 для второго мотора шагового ПРАВЫЙ

  HAL_TIM_Base_Start_IT(&htim6); // Таймер для общего цикла

  MX_I2C1_Init(); // Инициализация I2C1
  Scan_I2C_Universal();  // Сканируем шину, чтобы убедиться, что INA219 (0x40) и Мультиплексор (0x70) видны
  INA219_Init(); // Инициализация INA219 (после старта I2C)

  initMotor(); // Инициализация моторов
  testMotor(); // Тестирование правильности подключения моторов.
  
  initFirmware();
  // printf("Firmware gen %hu ver %hu laser %hu motor %.1f debug %hu\n", Print2Data_send.firmware.gen, Print2Data_send.firmware.ver,Print2Data_send.firmware.laser,Print2Data_send.firmware.motor,Print2Data_send.firmware.debug);

  initSPI_slave();    //
  timeSpi = millis(); // Запоминаем время начала цикла

  DEBUG_PRINTF("+++ LOOP !!!!!!!!!!!!!!!!!!!!!!!!!!! timeSpi= %lu \n", timeSpi);
  while (1)
  {
    workingSPI();   // Отработка действий по обмену по шине SPI
    workingTimer(); // Отработка действий по таймеру в 1, 50, 60 милисекунд

    // DEBUG_PRINTF("float %.2f Привет \n", 3.1415625);
    // HAL_GPIO_TogglePin(Led1_GPIO_Port, Led1_Pin);     // Инвертирование состояния выхода.
    // HAL_GPIO_TogglePin(Led2_GPIO_Port, Led2_Pin);     // Инвертирование состояния выхода.
    // HAL_GPIO_TogglePin(Analiz_GPIO_Port, Analiz_Pin); // Инвертирование состояния выхода.
    // HAL_Delay(500);
  }
}

#if DEBUG
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}
#endif

/**  * @brief System Clock Configuration  */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

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