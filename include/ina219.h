#ifndef __INA219_H__
#define __INA219_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

// Адрес устройства (A0=GND, A1=GND -> 0x40)
// HAL требует сдвига на 1 бит влево (0x40 << 1 = 0x80)
#define INA219_ADDRESS (0x40 << 1)

// Регистры INA219
#define INA219_REG_CONFIG 0x00
#define INA219_REG_SHUNTVOLTAGE 0x01
#define INA219_REG_BUSVOLTAGE 0x02
#define INA219_REG_POWER 0x03
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIBRATION 0x05

    // Структура для хранения данных с датчика
    typedef struct
    {
        float voltage_V;   // Напряжение на шине (Вольт)
        float current_mA;  // Ток (миллиАмпер)
        float power_mW;    // Мощность (миллиВатт)
        float shunt_mV;    // Падение на шунте (миллиВольт) - для отладки
        uint8_t connected; // 1 - датчик найден, 0 - ошибка
    } INA219_Data_t;

    // Глобальная переменная для доступа к данным
    extern INA219_Data_t ina219_data;

    // Функции
    uint8_t INA219_Init(void); // Инициализация и калибровка
    void INA219_Read(void);    // Чтение всех параметров (вызывать в таймере 10-50Гц)

#ifdef __cplusplus
}
#endif

#endif /* __INA219_H__ */