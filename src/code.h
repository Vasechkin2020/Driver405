#ifndef CODE_H
#define CODE_H

#define SPI_protocol yes

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h> // Для sinf и cosf

#include "slaveSPI.h"
#include "ina219.h"

extern void delayMotor(void); // <--- ВОТ ЭТОЙ СТРОЧКИ НЕ ХВАТАЛО
extern void setSpeed_L(float rps);
extern void setSpeed_R(float rps);
extern void movementTime();
extern void calcEncod(void); // Добавить прототип
//********************************* ПЕРЕМЕННЫЕ ***************************************************************************

bool flag_timer_10millisec = false;
bool flag_timer_50millisec = false;
bool flag_timer_1sec = false;

bool flagCAN = false;
uint32_t timeCAN = 0;

GPIO_TypeDef *myPort;

void timer6(); // Обработчик прерывания таймера TIM6	1 раз в 1 милисекунду

void workingTimer(); // Отработка действий по таймеру в 1, 50, 60 милисекунд
void workingSPI();   // Отработка действий по обмену по шине SPI
void initFirmware(); // Заполнение данными прошивки


uint32_t time_last_valid_command = 0; // Время получения последней валидной команды движения

HAL_StatusTypeDef status;
HAL_SPI_StateTypeDef statusGetState;

bool flagTimeOut = true;       // Флаг таймаута при обрыве связи по SPI
bool flagCallBackUart = false; // Флаг для указания нужно ли отрабатывать в колбеке  или обраьотка с самой функции

extern volatile uint32_t millisCounter;
//********************************* ФУНКЦИИ ***************************************************************************
// Функция для возврата количества миллисекунд
uint32_t millis()
{
    return millisCounter;
}

void timer6() // Обработчик прерывания таймера TIM6	1 раз в 1 милисекунду
{
    static int count_timer_10millisec = 0; // Счетчик для запуска обработки движения моторов в лупе по флагу
    static int count_timer_50millisec = 0; // Счетчик для запуска каждые 50 милисекунд
    static int count_timer_1sec = 0;       // Счетчик для запуска

    count_timer_10millisec++;
    count_timer_50millisec++;
    count_timer_1sec++;

    millisCounter++; // Увеличиваем счетчик миллисекунд

    //  каждые 10 милисекунд
    if (count_timer_10millisec >= 10)
    {
        count_timer_10millisec = 0;
        // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10);
        flag_timer_10millisec = true;
    }
    // каждые 50 милисекунд
    if (count_timer_50millisec >= 50)
    {
        count_timer_50millisec = 0;
        flag_timer_50millisec = true;
    }
    // 1 seconds
    if (count_timer_1sec >= 1000)
    {
        count_timer_1sec = 0;
        flag_timer_1sec = true;
    }
}

void workingTimer() // Отработка действий по таймеру в 1, 50, 60 милисекунд
{
    // HAL_Delay(); // Пауза 500 миллисекунд.
    //----------------------------- 10 миллисекунд --------------------------------------
    if (flag_timer_10millisec)
    {
        flag_timer_10millisec = false;
        // HAL_GPIO_TogglePin(Led1_GPIO_Port, Led1_Pin);             // Инвертирование состояния выхода.
        // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_10); // Инвертирование состояния выхода.
    }
    //----------------------------- 50 миллисекунд --------------------------------------
    if (flag_timer_50millisec)
    {
        flag_timer_50millisec = false;
        
        movementTime(); // 1. Проверка безопасности (если RPi молчит - стоп)
        delayMotor(); // Проверка простоя моторов

       INA219_Read(); // Опрос INA219 (займет ~0.5 мс в блокирующем режиме, это допустимо)

        // DEBUG_PRINTF("50msec %li \r\n", millis());
        //  flag_data = true; // Есть новые данные по шине // РУчной вариант имитации пришедших данных с частотой 20Гц
        // HAL_GPIO_TogglePin(Led1_GPIO_Port, Led1_Pin); // Инвертирование состояния выхода.
        // HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin); // Инвертирование состояния выхода.
    }

    //----------------------------- 1 секунда --------------------------------------
    if (flag_timer_1sec) // Вызывается каждую секунду
    {
        flag_timer_1sec = false;
        // statusGetState = HAL_SPI_GetState(&hspi1);
        // if (statusGetState == HAL_SPI_STATE_READY)
        // {
        //     DEBUG_PRINTF("Timer SPI. ПЕРЕЗАПУСК ПО ТАЙМЕРУ. ЭТО ОШИБКА\n");
        //     HAL_SPI_Abort(&hspi1);
        //     HAL_SPI_DMAStop(&hspi1);
        //     HAL_SPI_TransmitReceive_DMA(&hspi1, txBuffer, rxBuffer, BUFFER_SIZE); // // Перезапуск функции для следующего обмена// Запуск обмена данными по SPI с использованием DMA
        // }
        // else
        // {
        //     // DEBUG_PRINTF("Timer HAL_SPI_STATE_BUSY_TX_RX %u \n", statusGetState);
        // }
        DEBUG_PRINTF("%li \r\n", millis());
        //  uint8_t UART1_rxBuffer[4] = {0xAA,0xFF,0xAA,0xFF};
        //   uint8_t UART1_rxBuffer[1] = {0x56}; //Запрос версии "V"
        //   uint8_t UART1_rxBuffer[1] = {0x4F}; // Включить лазер "O"
        //   uint8_t UART1_rxBuffer[1] = {0x43}; // Выключить лазер "C"
    }
}

// Собираем нужные данные и пишем в структуру на отправку
// Аргумент restart:
// false - только обновить данные в буфере (для инициализации)
// true  - обновить данные И перезапустить DMA (для основного цикла)
void collect_Data_for_Send(bool restart)
{
    Driver2Data_send.id++;
    Driver2Data_send.spi = spi;

    uint32_t cheksum_send = 0;                                           // Считаем контрольную сумму отправляемой структуры
    unsigned char *adr_structura = (unsigned char *)(&Driver2Data_send); // Запоминаем адрес начала структуры. Используем для побайтной передачи
    for (int i = 0; i < sizeof(Driver2Data_send) - 4; i++)
    {
        cheksum_send += adr_structura[i]; // Побайтно складываем все байты структуры кроме последних 4 в которых переменная в которую запишем результат
    }
    Driver2Data_send.cheksum = cheksum_send;

    // копирование данных из моей уже заполненной структуры в буфер для DMA
    memset(txBuffer, 0, sizeof(txBuffer));                                            // Очистка буфера
    struct Struct_Driver2Data *copy_txBuffer = (struct Struct_Driver2Data *)txBuffer; // Создаем переменную в которую пишем адрес буфера в нужном формате
    *copy_txBuffer = Driver2Data_send;                                                // Копируем данные

    // 4. ПЕРЕЗАПУСК DMA (Только если попросили!)
    if (restart)
    {
        statusGetState = HAL_SPI_GetState(&hspi1);
        if (statusGetState == HAL_SPI_STATE_READY)
        {
            // DEBUG_PRINTF("SPI_GetState ok.\n");
            ;
        }
        else
            DEBUG_PRINTF("SPI_GetState ERROR %u ", statusGetState);

        status = HAL_SPI_TransmitReceive_DMA(&hspi1, txBuffer, rxBuffer, BUFFER_SIZE); // // Перезапуск функции для следующего обмена// Запуск обмена данными по SPI с использованием DMA                                       // Копируем из структуры данные в пвмять начиная с адреса в котором начинаяется буфер для передачи
        if (status == HAL_OK)
        {
            // DEBUG_PRINTF("DMA OK \n");
            ;
        }
        else
        {
            DEBUG_PRINTF("DMA ERROR \n");
            statusGetState = HAL_SPI_GetState(&hspi1);
            if (statusGetState == HAL_SPI_STATE_READY)
                DEBUG_PRINTF("2SPI ready to data transfer.\n");
            else
                DEBUG_PRINTF("2HAL_SPI_GetState ERROR %u \n", statusGetState);
        }
    }
}

// Отработка пришедших команд. Исполнение.
void executeDataReceive()
{
    // Статическая переменная хранит предыдущее состояние, чтобы сравнивать
    static struct SControl prevControl = {0.0f, 0.0f};

    // 1. Управление МОТОРАМИ
    // Проверяем: изменилась ли скорость ИЛИ скорость не нулевая
    // Это оптимизация: если мы стоим (0,0) и приходит снова (0,0) - не дергаем функции лишний раз.
    // Но если мы ехали, а пришло (0,0) - обязательно заходим внутрь, чтобы остановиться.
    if (Data2Driver_receive.control.speedL != prevControl.speedL || 
        Data2Driver_receive.control.speedR != prevControl.speedR ||
        Data2Driver_receive.control.speedL != 0.0f ||
        Data2Driver_receive.control.speedR != 0.0f)
    {
        // Обновляем таймер безопасности (Watchdog)
        time_last_valid_command = millis(); 

        // Применяем скорости
        setSpeed_L(Data2Driver_receive.control.speedL);
        setSpeed_R(Data2Driver_receive.control.speedR);
    }
    
    // Запоминаем текущее состояние как "предыдущее"
    prevControl = Data2Driver_receive.control;

    // 2. Управление СВЕТОДИОДАМИ
    // (Пока не реализовано, оставим место)
    // led_update(Data2Driver_receive.led);
}

// Отработка действий по обмену по шине SPI
void workingSPI()
{
    //----------------------------- По факту обмена данными с верхним уровнем --------------------------------------
#ifdef SPI_protocol
    if (flag_data) // Если обменялись данными
    {
        HAL_GPIO_WritePin(Analiz_0_GPIO_Port, Analiz_0_Pin, 1); // Инвертирование состояния выхода.
        // HAL_GPIO_WritePin(Analiz2_GPIO_Port, Analiz2_Pin, GPIO_PIN_SET); // Инвертирование состояния выхода.
        flag_data = false;
        flagTimeOut = true; // Флаг для выключения по таймауту
        timeSpi = millis(); // Запоминаем время обмена
        // HAL_GPIO_TogglePin(Analiz_0_GPIO_Port, Analiz_0_Pin); // Инвертирование состояния выхода.
        // DEBUG_PRINTF ("In = %#x %#x %#x %#x \r\n",rxBuffer[0],rxBuffer[1],rxBuffer[2],rxBuffer[3]);
        // DEBUG_PRINTF ("Out = %#x %#x %#x %#x \r\n",txBuffer[0],txBuffer[1],txBuffer[2],txBuffer[3]);
        // DEBUG_PRINTF("+\n");
        processingDataReceive(); // Обработка пришедших данных после состоявшегося обмена  !!! Подумать почему меняю данные даже если они с ошибкой, потом по факту когда будет все работать
        // DEBUG_PRINTF(" mode= %i \n",Data2Print_receive.controlMotor.mode);
        executeDataReceive(); // Выполнение пришедших команд

        // DEBUG_PRINTF(" Receive id= %i cheksum= %i command= %i ", Data2Print_receive.id, Data2Print_receive.cheksum,Data2Print_receive.command );
        // DEBUG_PRINTF("start = ");
        // for (int i = 0; i < sizeof(txBuffer); i++)
        // {
        //     DEBUG_PRINTF(" %x", txBuffer[i]);
        // }
        // DEBUG_PRINTF("\n");
        
        calcEncod(); // 3. РАСЧЕТ ТЕЛЕМЕТРИИ (НОВОЕ)
        collect_Data_for_Send(true); // Собираем данные в структуре для отправки на момент прихода команлы, но БЕЗ учета команды.До исполнения команды.

        // DEBUG_PRINTF(" angle0= %.2f angle1= %.2f angle2= %.2f angle3= %.2f", Data2Print_receive.angle[0], Data2Print_receive.angle[1], Data2Print_receive.angle[2], Data2Print_receive.angle[3] );

        // spi_slave_queue_Send();  // Закладываем данные в буфер для передачи(обмена)

        // DEBUG_PRINTF("end   = ");
        // for (int i = 0; i < sizeof(txBuffer); i++)
        // {
        //     DEBUG_PRINTF(" %x", txBuffer[i]);
        // }
        // DEBUG_PRINTF("-----\n");
        HAL_GPIO_WritePin(Analiz_0_GPIO_Port, Analiz_0_Pin, 0); // Инвертирование состояния выхода.
    }
#endif
}

// Заполнение данными Прошивки
void initFirmware()
{
    // Driver2Data_send.firmware.gen = 1;
    // Driver2Data_send.firmware.ver = 2;
    // Driver2Data_send.firmware.debug = DEBUG;
    // Driver2Data_send.firmware.test = 0x1A;
}

// Функция для включения FPU (Floating Point Unit) на STM32F4xx
void EnableFPU(void)
{
    // Включение FPU (CP10 и CP11: полный доступ)
    SCB->CPACR |= ((3UL << 20) | (3UL << 22)); // CP10 = 0b11, CP11 = 0b11

    if ((SCB->CPACR & (0xF << 20)) != (0xF << 20))
        printf("FPU отключена!\n");
    else
        printf("FPU включена!\n");

    uint32_t cpacr = SCB->CPACR; // Чтение регистра CPACR
    if ((cpacr & ((3UL << 20) | (3UL << 22))) == ((3UL << 20) | (3UL << 22)))
        printf("FPU2 включён\n");
    else
        printf("FPU2 отключён\n");

    float result = 0.0f;
    uint32_t start = HAL_GetTick();
    for (int i = 0; i < 10000; i++)
    {
        result += sinf((float)i / 100.0f);
        result += tanf((float)i / 100.0f);
    }
    uint32_t end = HAL_GetTick();
    uint32_t rez = end - start;
    printf("Time: %lu ms, Result: %f\n", rez, result);
    if (rez > 100)
        printf(" SOFT FPU !!!\n");
    else
        printf(" +++ HARD FPU !!!\n");
}

#endif /*CODE_H*/
