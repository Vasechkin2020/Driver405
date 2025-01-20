
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
    // ********************************************************************************************************

    //*********************************************************************
    struct motorStruct // Структура для локального управления и сбора данных по моторам
    {
        int status;                // Передаются импульсы на мотор или нет в данный момент, вращается или нет
        int position;              // Текущая позиция в импульсах
        int destination;           // Цель назначение в позиции в импульсах
        int dir;                   // Направление вращения мотора 1 - по часовой 0 - против часовой
        float angle;               // Углы в которые нужно повернультя в локальной системе
        GPIO_TypeDef *dir_port;    // Пин определяющий направление вращения
        uint16_t dir_pin;          // Пин определяющий направление вращения
        GPIO_TypeDef *step_port;   // Пин определяющий импульс
        uint16_t step_pin;         // Пин определяющий импульс
        GPIO_TypeDef *micric_port; // Пин определяющий концевик
        int micric_pin;            // Пин определяющий концевик
    };
    struct motorStruct motor; // Все локальные данные по моторам

    //*********************************************************************
    // Структура получаемых данных от Data к контроллеру Print
    struct SControlPrint
    {
        uint32_t status;    // Текущий режим работы 0 - не печатем, 1 печатаем
        uint32_t mode;      // Текущий режим работы какеи сопла печатют
        uint32_t intensity; // ИНтенсивность печати. Сколько раз прыскаем на 1 мм
        float speed;        // Текущая скорость движения при которой надо печатать. От нее зависит интервал между выпрыскиванием чернил
    };

    // Структура получаемых данных из Data к Print
    struct Struct_Data2Print
    {
        uint32_t id;                       // Id команды
        struct SControlPrint controlPrint; // Режим печати
        uint32_t cheksum;                  // Контрольная сумма данных в структуре
    };

    struct Struct_Data2Print Data2Print_receive;                               // Экземпляр структуры получаемых данных
    struct SControlPrint controlPrint;                                         // Управление всей печатью
    static const uint16_t size_structura_receive = sizeof(Data2Print_receive); // Размер структуры с данными которые получаем

    //*********************************************************************
    // Структура по состоянию лидаров которая передается на верхний уровень

    struct SFirmwarePrint
    {
        uint8_t gen;   // Поколение
        uint8_t ver;   // Версия
        uint8_t debug; // Вариант использования отладки для вывода в printf
        uint8_t test;  // Вариант использования отладки для вывода в printf
    };

    // Структура по обратной связи по обмену по шине SPI
    struct SSpi
    {
        uint32_t all;
        uint32_t bed;
    };
    struct SSpi spi; // Переменная где все данные по обмену

    // Структура состояния мотора GIM4305 4310
    struct SGim43
    {
        float position;
        float velocity;
        float torque;
    };
    

    // Структура в которой все главные переменные передаюся на высокий уровень от Print к Data
    struct Struct_Print2Data
    {
        uint32_t id;                    // id команды
        struct SFirmwarePrint firmware; // Версия прошики и использованного оборудования
        struct SGim43 gim43;
        struct SSpi spi;                // Структура по состоянию обмена по шине SPI

        uint32_t cheksum; // Контрольная сумма данных в структуре
    };

    struct Struct_Print2Data Print2Data_send;                            // Тут все переменные его характеризующие на низком уровне
    static const uint16_t size_structura_send = sizeof(Print2Data_send); // Размер структуры с данными которые передаем
    //static const uint16_t max_size_stuct = (size_structura_receive < size_structura_send) ? size_structura_send : size_structura_receive; // Какая из структур больше

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
