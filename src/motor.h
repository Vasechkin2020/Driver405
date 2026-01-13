#ifndef MOTOR_H
#define MOTOR_H

#include <math.h> // для abs()

// Параметры
#define MICROSTEP 8
#define REDUCTOR 5.0f
#define STEPS_PER_REV 200.0f // 1.8 градуса
#define MAX_RPS 3.0f

// Глобальные переменные состояния
volatile int32_t odom_ticks_L = 0;
volatile int32_t odom_ticks_R = 0;
int8_t dir_L = 0;
int8_t dir_R = 0;

uint32_t time_last_move = 0; // Время последней команды движения

// Флаги работы (нужны, чтобы не дергать пин, когда скорость 0)
bool motor_L_active = false;
bool motor_R_active = false;

// Инициализация
void initMotor()
{
    motor_L_active = false;
    motor_R_active = false;

    // Включаем таймеры для моторов
    HAL_TIM_Base_Start_IT(&htim7);
    HAL_TIM_Base_Start_IT(&htim13);

    // Включаем драйвер (En = 0 разрешает работу, если у тебя инверсно)
    // У тебя в коде: En_Motor_Pin SET (1). Проверь, 1 - это Enable или Disable?
    // Обычно 1 = Disable. Допустим, 0 = Enable.
    HAL_GPIO_WritePin(En_Motor_GPIO_Port, En_Motor_Pin, 1);
    // for (size_t i = 0; i < 1000; i++)
    // {
    //     // HAL_GPIO_TogglePin(En_Motor_GPIO_Port, En_Motor_Pin);
    //     HAL_GPIO_TogglePin(Dir_Motor0_GPIO_Port, Dir_Motor0_Pin);
    //     HAL_GPIO_TogglePin(Step_Motor0_GPIO_Port, Step_Motor0_Pin);
    //     HAL_GPIO_TogglePin(Dir_Motor1_GPIO_Port, Dir_Motor1_Pin);
    //     HAL_GPIO_TogglePin(Step_Motor1_GPIO_Port, Step_Motor1_Pin);
    //     HAL_GPIO_TogglePin(Analiz_0_GPIO_Port, Analiz_0_Pin);
    //     HAL_GPIO_TogglePin(Analiz_1_GPIO_Port, Analiz_1_Pin);
    //     DEBUG_PRINTF("    HAL_GPIO_TogglePin \n ");
    //     HAL_Delay(1000);
    // }
}
// Функция тестирования правильности подключения моторов.
void testMotor()
{
    // setSpeed_L(2); // Задаем напрямую скорость вращения обороты в секунду
    for (int i = 0; i < 5; i++)
    {
        setSpeed_L(i / 10.0);
        setSpeed_R(i / 10.0);
        DEBUG_PRINTF(" SpeedUP= %.2f \n", i / 10.0);
        HAL_Delay(250);
    }

    for (int j = 5; j >= 0; j--)
    {
        setSpeed_L(j / 10.0);
        setSpeed_R(j / 10.0);
        DEBUG_PRINTF(" SpeedDOWN= %.2f \n", j / 10.0);
        HAL_Delay(250);
    }
    // HAL_Delay(5000);

    // setSpeed_R(1.0);

    DEBUG_PRINTF(" While \n");
    HAL_GPIO_WritePin(En_Motor_GPIO_Port, En_Motor_Pin, 1); // Выключаем драйвер

    while (1)
        ;
}

void timerMotor_0() // Обработчик прерывания таймера TIM7 для ЛЕВОГО мотора
{
    // Логика шага
    if (motor_L_active)
    {
        // Делаем импульс (Очень быстро)
        Step_Motor0_GPIO_Port->BSRR = Step_Motor0_Pin; // High
        // Небольшая задержка (для некоторых драйверов нужна >1мкс)
        // На 168МГц пара тактов asm("nop") может понадобиться
        for (volatile int i = 0; i < 10; i++)
            ;
        Step_Motor0_GPIO_Port->BSRR = (uint32_t)Step_Motor0_Pin << 16U; // Low

        odom_ticks_L += dir_L; // Считаем одометрию
    }
}

void timerMotor_1() // Обработчик прерывания таймера TIM13 для ПРАВОГО мотора
{
    if (motor_R_active)
    {
        Step_Motor1_GPIO_Port->BSRR = Step_Motor1_Pin; // High
        for (volatile int i = 0; i < 10; i++)
            ;
        Step_Motor1_GPIO_Port->BSRR = (uint32_t)Step_Motor1_Pin << 16U; // Low

        odom_ticks_R += dir_R;
    }
}

// Функция установки скорости (Левый)
void setSpeed_L(float rps)
{
    // 1. Если скорость 0 -> Стоп
    if (fabs(rps) < 0.01f)
    {
        motor_L_active = false;
        return;
    }

    // 2. Расчет периода
    float steps_per_sec = fabs(rps) * STEPS_PER_REV * MICROSTEP * REDUCTOR;
    uint32_t period = (uint32_t)(1000000.0f / steps_per_sec);

    if (period < 20)
        period = 20; // Ограничение макс скорости
    if (period > 65000)
        period = 65000;

    // 3. Управление направлением
    if (rps > 0)
    {
        HAL_GPIO_WritePin(Dir_Motor0_GPIO_Port, Dir_Motor0_Pin, GPIO_PIN_SET);
        dir_L = 1;
    }
    else
    {
        HAL_GPIO_WritePin(Dir_Motor0_GPIO_Port, Dir_Motor0_Pin, GPIO_PIN_RESET);
        dir_L = -1;
    }

    // 4. Обновление скорости

    // Задаем новый период (попадет в теневой регистр и применится после Update Event)
    __HAL_TIM_SET_AUTORELOAD(&htim7, period);

    // 5. Старт с места
    // Если мотор спал - нужно его разбудить и сбросить счетчик,
    // чтобы не ждать окончания старого длинного цикла.
    if (!motor_L_active)
    {
        // Включаем драйвер физически
        HAL_GPIO_WritePin(En_Motor_GPIO_Port, En_Motor_Pin, GPIO_PIN_RESET);

        // Сброс счетчика только при старте!
        __HAL_TIM_SET_COUNTER(&htim7, 0);

        // Разрешаем генерацию шагов
        motor_L_active = true;
    }

    // Обновляем время активности для delayMotor()
    time_last_move = millis();
}

// Функция установки скорости (Правый)
void setSpeed_R(float rps)
{
    if (fabs(rps) < 0.01f)
    {
        motor_R_active = false;
        return;
    }

    float steps_per_sec = fabs(rps) * STEPS_PER_REV * MICROSTEP * REDUCTOR;
    uint32_t period = (uint32_t)(1000000.0f / steps_per_sec);

    if (period < 20)
        period = 20;
    if (period > 65000)
        period = 65000;

    if (rps > 0)
    {
        HAL_GPIO_WritePin(Dir_Motor1_GPIO_Port, Dir_Motor1_Pin, GPIO_PIN_RESET);
        dir_R = 1;
    }
    else
    {
        HAL_GPIO_WritePin(Dir_Motor1_GPIO_Port, Dir_Motor1_Pin, GPIO_PIN_SET);
        dir_R = -1;
    }

    __HAL_TIM_SET_AUTORELOAD(&htim13, period);

    if (!motor_R_active)
    {
        HAL_GPIO_WritePin(En_Motor_GPIO_Port, En_Motor_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COUNTER(&htim13, 0);
        motor_R_active = true;
    }

    time_last_move = millis();
}

void delayMotor()
{
    // Если моторы стоят (active = false) И прошло больше 2000 мс
    if (!motor_L_active && !motor_R_active)
    {
        if ((millis() - time_last_move) > 2000)
        {
            // Отключаем питание драйверов (экономия энергии)
            HAL_GPIO_WritePin(En_Motor_GPIO_Port, En_Motor_Pin, GPIO_PIN_SET); // 1 = Disable
        }
    }
}

void movementTime()
{
    // Если с момента последней команды прошло больше 1000 мс (1 сек)
    if ((millis() - time_last_valid_command) > 1000)
    {
        // И если моторы все еще активны
        if (motor_L_active || motor_R_active)
        {
            // Принудительно останавливаем
            setSpeed_L(0);
            setSpeed_R(0);
            DEBUG_PRINTF("Safety Stop: No commands for 1s\r\n");
        }
    }
}

extern struct Struct_Driver2Data Driver2Data_send; // Чтобы писать прямо в структуру отправки

void calcEncod()
{
    static uint32_t time_prev = 0;
    uint32_t time_now = millis();

    if (time_prev == 0)
    {
        time_prev = time_now;
        return;
    }

    float dt = (float)(time_now - time_prev) / 1000.0f;
    if (dt < 0.01f)
        return; // Защита от слишком частого вызова

    time_prev = time_now;

    // --- Атомарное чтение ---
    int32_t ticks_L_snapshot, ticks_R_snapshot;
    __disable_irq();
    ticks_L_snapshot = odom_ticks_L;
    odom_ticks_L = 0;
    ticks_R_snapshot = odom_ticks_R;
    odom_ticks_R = 0;
    __enable_irq();

    // --- РАСЧЕТ КАК НА ESP32 (Скорость вала мотора) ---
    // Формула ESP32: ticks * (1.8 / micro) / 360 / dt
    // Упрощаем: ticks / (micro * 200 * dt)
    // REDUCTOR здесь НЕ участвует, чтобы совпадать со старым проектом!

    float steps_per_motor_rev = STEPS_PER_REV * MICROSTEP; // 200 * 8 = 1600

    Driver2Data_send.motor.rpsEncodL = ((float)ticks_L_snapshot / dt) / steps_per_motor_rev;
    Driver2Data_send.motor.rpsEncodR = ((float)ticks_R_snapshot / dt) / steps_per_motor_rev;

    // --- СТАТУС ДРАЙВЕРА ---
    // 1 - выключен (SET), 0 - включен (RESET)
    GPIO_PinState state = HAL_GPIO_ReadPin(En_Motor_GPIO_Port, En_Motor_Pin);
    Driver2Data_send.motor.statusDriver = (state == GPIO_PIN_SET) ? 1 : 0;
}

// ЭТА ВЕРСИЯ ВОЗВРАЩАЕТ СКОРОСТЬ КОЛЕСА, А ТА ЧТО ВЫШЕ - СКОРОСТЬ ВАЛА МОТОРА. НЕ ЗНАЮ ПОЧЕМУ ТАК НАПИСАНО БЫЛО ИЗНАЧАЛЬНО.
// void calcEncod()
// {
//     static uint32_t time_prev = 0;
//     uint32_t time_now = millis();

//     // Защита от деления на ноль при первом запуске
//     if (time_prev == 0) {
//         time_prev = time_now;
//         return;
//     }

//     // Вычисляем dt в секундах
//     // (time_now - time_prev) дает миллисекунды, делим на 1000.0
//     float dt = (float)(time_now - time_prev) / 1000.0f;

//     // Если прошло слишком мало времени (меньше 10мс), выходим, чтобы не плодить шумы
//     if (dt < 0.01f) return;

//     time_prev = time_now;

//     // --- АТОМАРНОЕ ЧТЕНИЕ И СБРОС ---
//     // Нам нужно забрать тики и обнулить переменную.
//     // Пока мы это делаем, прерывание не должно вклиниться.

//     int32_t ticks_L_snapshot, ticks_R_snapshot;

//     __disable_irq(); // Выключаем прерывания (критическая секция)
//     ticks_L_snapshot = odom_ticks_L;
//     odom_ticks_L = 0; // Сбрасываем накопитель
//     ticks_R_snapshot = odom_ticks_R;
//     odom_ticks_R = 0; // Сбрасываем накопитель
//     __enable_irq();  // Включаем прерывания обратно

//     // --- РАСЧЕТ СКОРОСТИ (RPS) ---
//     // Формула: (Тики / dt) / (Шагов на оборот * Микрошаг * Редуктор)
//     float steps_per_rev_total = STEPS_PER_REV * MICROSTEP * REDUCTOR;

//     // Записываем прямо в структуру для отправки
//     Driver2Data_send.motor.rpsEncodL = ((float)ticks_L_snapshot / dt) / steps_per_rev_total;
//     Driver2Data_send.motor.rpsEncodR = ((float)ticks_R_snapshot / dt) / steps_per_rev_total;

//     // --- СТАТУС ДРАЙВЕРА ---
//     // Читаем состояние пина EN.
//     // Если пин RESET (0) -> Драйвер включен -> status = 0 (В старом коде: 0-Работа, 1-Выкл)
//     // Если пин SET (1)   -> Драйвер выключен -> status = 1
//     GPIO_PinState state = HAL_GPIO_ReadPin(En_Motor_GPIO_Port, En_Motor_Pin);
//     Driver2Data_send.motor.statusDriver = (state == GPIO_PIN_SET) ? 1 : 0;

//     // Для отладки можно вывести
//     // DEBUG_PRINTF("Odom: L=%.2f R=%.2f\n", Driver2Data_send.motor.rpsEncodL, Driver2Data_send.motor.rpsEncodR);
// }

#endif /*MOTOR_H*/