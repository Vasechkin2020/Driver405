#include "ina219.h"
#include "i2c.h" // Чтобы видеть hi2c1
#include <stdio.h> // <--- ДОБАВИТЬ ВОТ ЭТО для printf

// Глобальный экземпляр структуры
INA219_Data_t ina219_data = {0};

// Значения калибровки (для шунта 0.1 Ом)
// Current LSB = 100uA = 0.1mA
// Cal = 0.04096 / (Current_LSB * R_Shunt) = 0.04096 / (0.0001 * 0.1) = 4096
// 4096 в hex = 0x1000
static const uint16_t INA219_CAL_VALUE = 0x1000;

// Конфигурация:
// Voltage Range 32V (Bus Voltage Range = 1)
// Gain /8 (Shunt Voltage Range = 11 -> +/- 320mV)
// ADC 12-bit (Bus & Shunt ADC = 0011)
// Mode Continuous (Mode = 111)
// Итого: 0011 1001 1001 1111 = 0x399F
static const uint16_t INA219_CONFIG_VALUE = 0x399F;

// Вспомогательная функция записи 16-бит регистра (Big Endian)
static HAL_StatusTypeDef INA219_WriteRegister(uint8_t reg, uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF; // Старший байт
    data[1] = value & 0xFF;        // Младший байт
    return HAL_I2C_Mem_Write(&hi2c1, INA219_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, 2, 10);
}

// Вспомогательная функция чтения 16-бит регистра
static int16_t INA219_ReadRegister(uint8_t reg)
{
    uint8_t data[2];
    if (HAL_I2C_Mem_Read(&hi2c1, INA219_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, data, 2, 10) != HAL_OK)
    {
        return 0; // Ошибка чтения
    }
    return (int16_t)((data[0] << 8) | data[1]); // Собираем 16 бит
}

// Инициализация датчика
uint8_t INA219_Init(void)
{
    // 1. Проверяем, есть ли устройство на шине
    if (HAL_I2C_IsDeviceReady(&hi2c1, INA219_ADDRESS, 2, 10) != HAL_OK)
    {
        ina219_data.connected = 0;
        printf("INA219 not found!\r\n");
        return 0; // Датчик не найден
    }

    // 2. Калибровка (ОБЯЗАТЕЛЬНО для чтения тока)
    // Сначала пишем калибровку
    if (INA219_WriteRegister(INA219_REG_CALIBRATION, INA219_CAL_VALUE) != HAL_OK) return 0;
    
    // 3. Конфигурация (32V, 3.2A)
    if (INA219_WriteRegister(INA219_REG_CONFIG, INA219_CONFIG_VALUE) != HAL_OK) return 0;

    ina219_data.connected = 1;
    printf("INA219 Init OK.\r\n");
    return 1;
}

// Чтение данных (вызывать периодически)
void INA219_Read(void)
{
    if (!ina219_data.connected) return; // Если датчика нет, не мучаем I2C

    const float koef = 0.933; // Коэффициент для коррекции погрешности измерения тока и мощности
    int16_t raw_bus, raw_shunt, raw_current, raw_power;

    // 1. Читаем Bus Voltage (Регистр 0x02)
    // Биты 3-15 хранят значение. Младшие 3 бита (0-2) не нужны или статус. Сдвигаем на 3.
    // 1 LSB = 4 mV
    raw_bus = INA219_ReadRegister(INA219_REG_BUSVOLTAGE);
    int16_t bus_value = (raw_bus >> 3); 
    ina219_data.voltage_V = bus_value * 0.004f; 

    // 2. Читаем Shunt Voltage (Регистр 0x01)
    // 1 LSB = 10 uV = 0.01 mV
    raw_shunt = INA219_ReadRegister(INA219_REG_SHUNTVOLTAGE);
    ina219_data.shunt_mV = raw_shunt * 0.01f;

    // --- ТРЮК ADAFRUIT: ВОССТАНОВЛЕНИЕ КАЛИБРОВКИ ---
    // На случай, если чип сбросился от помехи моторов
    INA219_WriteRegister(INA219_REG_CALIBRATION, INA219_CAL_VALUE);

    // 3. Читаем Current (Регистр 0x04)
    // Мы калибровали так, что Current LSB = 0.1 mA
    raw_current = INA219_ReadRegister(INA219_REG_CURRENT);
    // ina219_data.current_mA = raw_current * 0.1f; // 0.1mA на бит
    ina219_data.current_mA = raw_current * 0.2f * koef; // <--- ИСПРАВИТЬ ЗДЕСЬ для 6,4 А

    // 4. Читаем Power (Регистр 0x03)
    // Power LSB = 20 * Current LSB = 2 mW
    raw_power = INA219_ReadRegister(INA219_REG_POWER);
    // ina219_data.power_mW = raw_power * 2.0f;
    ina219_data.power_mW = raw_power * 4.0f * koef;     // <--- И ЗДЕСЬ  для 6,4 А
    
    // printf("+++ Ina219 V=%7.2f I=%7.2f P=%7.2f\n", ina219_data.voltage_V, ina219_data.current_mA, ina219_data.power_mW);
}