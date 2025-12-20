#ifndef MOTOR_H
#define MOTOR_H

#include <math.h> // для abs()


void initMotor();                                            // Функция инциализации моторов


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

// Флаги работы (нужны, чтобы не дергать пин, когда скорость 0)
bool motor_L_active = false;
bool motor_R_active = false;

// Инициализация
void initMotor()
{
    // Включаем таймеры (но пока без генерации шагов, период огромный)
    HAL_TIM_Base_Start_IT(&htim7);
    HAL_TIM_Base_Start_IT(&htim13);
    
    // Включаем драйвер (En = 0 разрешает работу, если у тебя инверсно)
    // У тебя в коде: En_Motor_Pin SET (1). Проверь, 1 - это Enable или Disable?
    // Обычно 1 = Disable. Допустим, 0 = Enable.
    HAL_GPIO_WritePin(En_Motor_GPIO_Port, En_Motor_Pin, GPIO_PIN_RESET); 
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
    if (fabs(rps) < 0.01f) {
        motor_L_active = false;
        return;
    }

    // Направление
    if (rps > 0) {
        HAL_GPIO_WritePin(Dir_Motor0_GPIO_Port, Dir_Motor0_Pin, GPIO_PIN_RESET); // Вперед
        dir_L = 1;
    } else {
        HAL_GPIO_WritePin(Dir_Motor0_GPIO_Port, Dir_Motor0_Pin, GPIO_PIN_SET);   // Назад
        dir_L = -1;
    }

    // Расчет периода таймера (ARR)
    // RPS * 200 * 8 * 5 = Шагов в секунду
    float steps_per_sec = fabs(rps) * STEPS_PER_REV * MICROSTEP * REDUCTOR;
    
    // Timer Tick = 1 MHz (1 000 000 тиков в сек)
    // ARR = 1 000 000 / Steps_per_sec
    uint32_t period = (uint32_t)(1000000.0f / steps_per_sec);
    
    if (period < 10) period = 10; // Защита от сверхвысоких частот
    if (period > 65000) period = 65000;

    // Обновляем таймер
    __HAL_TIM_SET_AUTORELOAD(&htim7, period);
    motor_L_active = true;
}

// Функция установки скорости (Правый)
void setSpeed_R(float rps)
{
    if (fabs(rps) < 0.01f) {
        motor_R_active = false;
        return;
    }

    if (rps > 0) {
        HAL_GPIO_WritePin(Dir_Motor1_GPIO_Port, Dir_Motor1_Pin, GPIO_PIN_RESET);
        dir_R = 1;
    } else {
        HAL_GPIO_WritePin(Dir_Motor1_GPIO_Port, Dir_Motor1_Pin, GPIO_PIN_SET);
        dir_R = -1;
    }

    float steps_per_sec = fabs(rps) * STEPS_PER_REV * MICROSTEP * REDUCTOR;
    uint32_t period = (uint32_t)(1000000.0f / steps_per_sec);
    
    if (period < 10) period = 10;
    if (period > 65000) period = 65000;

    __HAL_TIM_SET_AUTORELOAD(&htim13, period);
    motor_R_active = true;
}



























#endif /*MOTOR_H*/