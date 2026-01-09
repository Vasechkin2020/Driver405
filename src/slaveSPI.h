/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SLAVE_SPI_H__
#define __SLAVE_SPI_H__

#include "main.h"
#include "dma.h"
#include "spi.h"

#define BUFFER_SIZE 96 // Размер буфера который передаем. Следить что-бы структуры не превышали этот размер Кратно 32 делать
// uint8_t txBuffer[BUFFER_SIZE] = {0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xA0}; // = "Hello from STM32 Slave"; // Передающий буфер

uint8_t txBuffer[BUFFER_SIZE] = {0}; // = "Hello from STM32 Slave"; // Передающий буфер
uint8_t rxBuffer[BUFFER_SIZE];       // Принимающий буфер

uint32_t timeSpi = 0; // Время когда пришла команда по SPI

extern SPI_HandleTypeDef hspi1;
extern uint32_t millis();
volatile bool flag_data = false; // Флаг что данные передались

//********************** ОБЯВЛЕНИЕ ФУНКЦИЙ ================================

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi); // Обработчик прерывания при завершении обмена данных по DMA
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);    // Обработка ошибок SPI
void initSPI_slave();                                   // Начальная инициализция для SPI
// void spi_slave_queue_Send();                            // Функция в которой чистим буфер и закладываем данные на передачу в буфер
void processingDataReceive(); // Обработка по флагу в main пришедших данных после срабатывания прерывания что обмен состоялся

//********************** РЕАЛИЗАЦИЯ ФУНКЦИЙ ================================

// Функция возвращает контрольную сумму структуры без последних 4 байтов
uint32_t measureCheksum_Data2Driver(const struct Struct_Data2Driver *structura_)
{
    uint32_t ret = 0;
    unsigned char *adr_structura = (unsigned char *)(structura_); // Запоминаем адрес начала структуры. Используем для побайтной передачи
    for (int i = 0; i < sizeof(structura_) - 4; i++)
    {
        ret += adr_structura[i]; // Побайтно складываем все байты структуры кроме последних 4 в которых переменная в которую запишем результат
    }
    return ret;
}
// Функция возвращает контрольную сумму структуры без последних 4 байтов
uint32_t measureCheksum_Driver2Data(const struct Struct_Driver2Data *structura_)
{
    uint32_t ret = 0;
    unsigned char *adr_structura = (unsigned char *)(structura_); // Запоминаем адрес начала структуры. Используем для побайтной передачи
    for (int i = 0; i < sizeof(structura_) - 4; i++)
    {
        ret += adr_structura[i]; // Побайтно складываем все байты структуры кроме последних 4 в которых переменная в которую запишем результат
    }
    return ret;
}

// Обработчик прерывания при завершении обмена данных по DMA
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1)
    {
        // HAL_GPIO_WritePin(Analiz_GPIO_Port, Analiz_Pin, GPIO_PIN_SET); // Инвертирование состояния выхода.
        flag_data = true; // Флаг что обменялись данными. По этому флагу происходит обработка полученных данных и подготовка данных к следующей передаче
        // DEBUG_PRINTF("-up-\n");
        // HAL_GPIO_TogglePin(Led2_GPIO_Port, Led2_Pin);                    // Инвертирование состояния выхода.
        spi.all++; // Считаем сколько было обменов данными всего
        // HAL_GPIO_WritePin(Analiz_GPIO_Port, Analiz_Pin, GPIO_PIN_RESET); // Инвертирование состояния выхода.

        // //копировнаие данных из моей уже заполненной структуры в буфер для DMA
        // memset(txBuffer, 0, sizeof(txBuffer)); // Очистка буфера
        // struct Struct_Print2Data *copy_txBuffer = (struct Struct_Print2Data *)txBuffer; // Создаем переменную в которую пишем адрес буфера в нужном формате
        // *copy_txBuffer = Print2Data_send; // Копируем данные

        // HAL_SPI_TransmitReceive_DMA(&hspi1, txBuffer, rxBuffer, BUFFER_SIZE); // // Перезапуск функции для следующего обмена// Запуск обмена данными по SPI с использованием DMA
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    DEBUG_PRINTF("Ошибка SPI, код: 0x%08lx\n", hspi->ErrorCode);

    if (hspi->ErrorCode & HAL_SPI_ERROR_MODF)
    {
        DEBUG_PRINTF("Ошибка: Mode Fault (MODF).\n");
    }
    if (hspi->ErrorCode & HAL_SPI_ERROR_CRC)
    {
        DEBUG_PRINTF("Ошибка: CRC Error.\n");
    }
    if (hspi->ErrorCode & HAL_SPI_ERROR_OVR)
    {
        DEBUG_PRINTF("Ошибка: Overrun.\n");
        // // Сброс OVR: прочитать DR и SR
        // volatile uint32_t temp = hspi->Instance->DR;
        // temp = hspi->Instance->SR;
        // (void)temp;

        // OVR блокирует SPI. Нужно его сбросить. Макрос __HAL_SPI_CLEAR_OVRFLAG делает чтение DR и SR регистров
        __HAL_SPI_CLEAR_OVRFLAG(hspi);
        DEBUG_PRINTF("Err: OVR fixed.\n");
    }
    if (hspi->ErrorCode & HAL_SPI_ERROR_DMA)
    {
        DEBUG_PRINTF("Ошибка: DMA Error.\n");
    }
    if (hspi->ErrorCode & HAL_SPI_ERROR_FLAG)
    {
        DEBUG_PRINTF("Ошибка: Общая ошибка флагов.\n");
        // Сброс MODF: повторное включение SPI
        __HAL_SPI_DISABLE(hspi);
        __HAL_SPI_ENABLE(hspi);
    }
    if (hspi->ErrorCode == HAL_SPI_ERROR_NONE)
    {
        DEBUG_PRINTF("Ошибок нет.\n");
    }

    // На всякий случай делаем Abort, чтобы сбросить состояние HAL
    HAL_SPI_Abort(hspi);
    DEBUG_PRINTF("HAL_SPI_Abort: На всякий случай делаем Abort, чтобы сбросить состояние HAL.\n");

    // Перезапускаем
    HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, BUFFER_SIZE);
    DEBUG_PRINTF("HAL_SPI_TransmitReceive_DMA:После устранения ошибок.\n");
}

extern void collect_Data_for_Send(bool restart);

// Начальная инициализция для SPI
void initSPI_slave()
{
    // 1. Готовим данные для САМОЙ ПЕРВОЙ отправки (чтобы не слать нули)
    collect_Data_for_Send(false); // false = не перезапускать DMA, просто заполнить буфер

    __HAL_SPI_CLEAR_OVRFLAG(&hspi1); // Добавляем очистку флагов перед стартом, чтобы убрать мусор

    // 2. Взводим курок в первый раз. Теперь STM32 готов и ждет, когда RPi начнет общение.
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, txBuffer, rxBuffer, BUFFER_SIZE);
    if (status == HAL_OK)
    {
        DEBUG_PRINTF("Start first DMA OK \n");
    }
}

// Обработка по флагу в main пришедших данных после срабатывания прерывания что обмен состоялся
void processingDataReceive()
{
    // struct STest
    // {
    //     uint8_t byte0;
    //     uint8_t byte1;
    //     uint8_t byte2;
    //     uint8_t byte3;
    //     float fff;
    // };

    // struct STest StructTestPSpi;

    // struct STest StructTestPSpi_temp;                       // Экземпляр структуры получаемых данных временный, пока не посчитаем контроьную сумму и убедимся что данные хорошие
    // struct STest *copy_rxBuffer = (struct STest *)rxBuffer; // Создаем переменную в которую пишем адрес буфера в нужном формате
    // StructTestPSpi_temp = *copy_rxBuffer;                   // Копируем из этой перемнной данные в мою структуру

    struct Struct_Data2Driver Data2Driver_receive_temp;                               // Экземпляр структуры получаемых данных временный, пока не посчитаем контроьную сумму и убедимся что данные хорошие
    struct Struct_Data2Driver *copy_rxBuffer = (struct Struct_Data2Driver *)rxBuffer; // Создаем переменную в которую пишем адрес буфера в нужном формате
    Data2Driver_receive_temp = *copy_rxBuffer;                                        // Копируем из этой перемнной данные в мою структуру

    uint32_t cheksum_receive = 0; // = measureCheksum(Data2Driver_receive_temp);             // Считаем контрольную сумму пришедшей структуры

    // unsigned char *adr_structura = (unsigned char *)(&StructTestPSpi_temp); // Запоминаем адрес начала структуры. Используем для побайтной передачи

    unsigned char *adr_structura = (unsigned char *)(&Data2Driver_receive_temp); // Запоминаем адрес начала структуры. Используем для побайтной передачи
    for (int i = 0; i < sizeof(Data2Driver_receive_temp) - 4; i++)
    {
        cheksum_receive += adr_structura[i]; // Побайтно складываем все байты структуры кроме последних 4 в которых переменная в которую запишем результат
    }

    if (cheksum_receive != Data2Driver_receive_temp.cheksum || Data2Driver_receive_temp.cheksum == 0) // Стравниваю что сам посчитал и что прислали. Не сходится или ноль - значит плохие данные
    {
        spi.bed++; // Плохие данные
        DEBUG_PRINTF("IN Data Err. Exp: %lu, Got: %lu\n", Data2Driver_receive_temp.cheksum, cheksum_receive);
        DEBUG_PRINTF("DUMP: "); // ВЫВОДИМ БАЙТЫ, ЧТОБЫ ПОНЯТЬ ПРИЧИНУ
        for (int i = 0; i < BUFFER_SIZE; i++) // Мой буфер 96 байт
        { 
            DEBUG_PRINTF("%02X ", rxBuffer[i]);
        }
        DEBUG_PRINTF("\n");

        // DEBUG_PRINTF(" IN Data Err. \n");
        // for (int i = 0; i < 8; i++)
        // {
        //     DEBUG_PRINTF("%#X ", adr_structura[i]);
        // }
        // DEBUG_PRINTF(" /// ");
        // for (int i = 8; i < 12; i++)
        // {
        //     DEBUG_PRINTF("%#X ", adr_structura[i]);
        // }
        // DEBUG_PRINTF("||| %#x != %#x", cheksum_receive, Data2Driver_receive_temp.cheksum);
        // Внутри processingDataReceive() или в workingSPI при ошибке:
    }
    else
    {
        Data2Driver_receive = Data2Driver_receive_temp; // Хорошие данные копируем
        // DEBUG_PRINTF("%#x != %#x", cheksum_receive, Data2Driver_receive_temp.cheksum);
        DEBUG_PRINTF("%li msec | IN Data OK. \n", millis());
    }
    // DEBUG_PRINTF(" All= %lu bed= %lu \r\n", spi.all, spi.bed);
    // DEBUG_PRINTF("b1 = %#X b2 = %#X b3 = %#X b4 = %#X %.4f = ", StructTestPSpi_temp.byte0, StructTestPSpi_temp.byte1, StructTestPSpi_temp.byte2, StructTestPSpi_temp.byte3, StructTestPSpi_temp.fff);
    //  for (int i = 0; i < sizeof(Data2Print_receive); i++)
    //  {
    //      DEBUG_PRINTF("%#X ", adr_structura[i]);
    //  }
    memset(rxBuffer, 0, sizeof(rxBuffer)); // Чистим буфер
}
#endif /* __SPI_H__ */
