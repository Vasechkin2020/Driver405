
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f4xx_hal.h"

    void Error_Handler(void);

#define Led_Pin GPIO_PIN_3
#define Led_GPIO_Port GPIOC
#define Analiz_Pin GPIO_PIN_9
#define Analiz_GPIO_Port GPIOC

    // Установите DEBUG для включения отладочной информации
#define DEBUG 1 // Поставьте 0 для отключения отладочной информации

#if DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
    // #define DEBUG_PRINTF(fmt,...) printf("GREEN" fmt , ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) (void)0 // Приведение 0 к типу void, ничего не делает
#endif
    //****************************************************************************************************************************************************
    // Структура по обратной связи по обмену по шине SPI

    struct SXyz //
    {
        float x;
        float y;
        float z;
    };
    struct SSpi
    {
        uint32_t all;
        uint32_t bed;
    };
    //****************************************************************************************************************************************************
    // Структура управления движением
    struct SControl
    {
        float speedL; // Скорость с которой нужно двигаться в оборотах/секунда !!!!!!!!!
        float speedR; // Скорость с которой нужно двигаться в оборотах/секунда !!!!!!!!!!!!!!!!
    };

    #define NUM_LEDS 43 // Колличество светодиодов всего

    // Структура управления Светодиодами
    struct SLed
    {
        uint8_t led[NUM_LEDS]; // Маасив через который управляем светодиодами
    };
    // Структура получаемых данных от Data к контроллеру Driver
    struct Struct_Data2Driver
    {
        uint32_t id;      // Номер команды по порядку
        struct SControl control; // Структура управления машиной
        struct SLed led;         // Управление светодиодами
        uint32_t cheksum; // Контрольная сумма данных в структуре
    };

    struct Struct_Data2Driver Data2Driver_receive;                         // Экземпляр структуры получаемых данных
    static const int size_structura_receive = sizeof(Data2Driver_receive); // Размер структуры с данными которые получаем

    //*****************************************************************************************************************************************************
    // Структура сосдержит всю информацию по мотору на основании данных энкодера
    struct SMotor
    {
        uint32_t statusDriver; // Статус драйвера.Включен или выключен.Удерживаются колосеса или свободно катаются 1-выключен
        float rpsEncodL;       // Реальная скорость вращения по енкодерам( обороты в секунду)
        float rpsEncodR;       // Реальная скорость вращения по енкодерам( обороты в секунду)
    };

    struct SMpu // Структура с данными со всех датчиков, отправляем наверх
    {
        int32_t status; // статус состояния
        float rate;     // частота работы датчика
        struct SXyz angleEuler;
        struct SXyz linear;
    };

    // Структура состояния датчика расстония
    struct SSensor
    {
        int32_t status; // статус состояния
        float distance; // расстояние до препятствия
    };
    // Отдельные структуры для каждой сущности
    struct SMotor motor;
    struct SMpu bno055; // Данные с датчика BNO055
    struct SSensor laserL;
    struct SSensor laserR;
    struct SSensor uzi;
    struct SSpi spi; // Структура по состоянию обмена по шине SPI

    // Структура в которой все главные переменные передаюся на высокий уровень
    struct Struct_Driver2Data
    {
        uint32_t id; // id команды
        struct SMotor motor;
        struct SMpu bno055; // Данные с датчика BNO055
        struct SSensor laserL;
        struct SSensor laserR;
        struct SSensor uzi;
        struct SSpi spi;             // Структура по состоянию обмена по шине SPI
        uint32_t cheksum; // Контрольная сумма данных в структуре
    };

    struct Struct_Driver2Data Driver2Data_send;                                                                                           // Тело робота. тут все переменные его характеризующие на низком уровне
    static const int size_structura_send = sizeof(Driver2Data_send);                                                                      // Размер структуры с данными которые передаем
    // static const uint16_t max_size_stuct = (size_structura_receive < size_structura_send) ? size_structura_send : size_structura_receive; // Какая из структур больше

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
