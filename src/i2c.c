
#include "i2c.h"
#include <stdio.h> // <--- ДОБАВИТЬ ВОТ ЭТО для printf

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = SCL_Pin|SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(SCL_GPIO_Port, SCL_Pin);

    HAL_GPIO_DeInit(SDA_GPIO_Port, SDA_Pin);
  }
}

//*****************************************************************************************************

// Адрес мультиплексора TCA9548A (7-бит: 0x70 -> 8-бит: 0xE0)
#define TCA9548A_ADDR (0x70 << 1) 

// Вспомогательная функция переключения канала
static uint8_t TCA9548A_SelectChannel(uint8_t channel)
{
    if (channel > 7) return 0;
    
    uint8_t data = (1 << channel); // Бит соответствует каналу
    // Отправляем 1 байт с битовой маской
    HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(&hi2c1, TCA9548A_ADDR, &data, 1, 10);
    
    return (res == HAL_OK);
}

// Вспомогательная функция сканирования текущей активной шины
static void Scan_Segment(char *segment_name)
{
    printf("--- Scanning: %s ---\r\n", segment_name);
    int found = 0;

    // Сканируем стандартные адреса (0x08 ... 0x77)
    for (uint16_t addr = 0x08; addr < 0x78; addr++)
    {
        // Не сканируем сам мультиплексор во избежание путаницы, 
        // хотя он будет виден на всех каналах
        if (addr == 0x70) continue; 

        // Проверяем устройство (Trials=1, Timeout=10ms)
        if (HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 1, 10) == HAL_OK)
        {
            printf("  [0x%02X] Device Found!\r\n", addr);
            found++;
        }
    }

    if (found == 0) printf("  No devices.\r\n");
}

// ГЛАВНАЯ УНИВЕРСАЛЬНАЯ ФУНКЦИЯ
void Scan_I2C_Universal(void)
{
    printf("\r\n=== I2C BUS SCANNER START ===\r\n");

    // 1. Проверяем наличие мультиплексора
    HAL_StatusTypeDef mux_status = HAL_I2C_IsDeviceReady(&hi2c1, TCA9548A_ADDR, 2, 10);

    if (mux_status != HAL_OK)
    {
        // === ВАРИАНТ 1: Мультиплексора нет ===
        printf("Multiplexer NOT found at 0x70. Scanning Main Bus only.\r\n");
        Scan_Segment("Main Bus");
    }
    else
    {
        // === ВАРИАНТ 2: Мультиплексор найден ===
        printf("Multiplexer FOUND at 0x70. Scanning Channels 0-7...\r\n");

        for (uint8_t ch = 0; ch < 8; ch++)
        {
            char buff[16];
            
            // Переключаем канал
            if (TCA9548A_SelectChannel(ch))
            {
                snprintf(buff, sizeof(buff), "Channel %d", ch);
                // Небольшая задержка для стабилизации переключения (опционально)
                HAL_Delay(1); 
                Scan_Segment(buff);
            }
            else
            {
                printf("Error switching to Channel %d\r\n", ch);
            }
        }
        
        // В конце отключаем все каналы (пишем 0)
        uint8_t off = 0;
        HAL_I2C_Master_Transmit(&hi2c1, TCA9548A_ADDR, &off, 1, 10);
    }

    printf("=== I2C SCANNER END ===\r\n\r\n");
}