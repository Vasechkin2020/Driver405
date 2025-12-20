
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "main.h"

    extern TIM_HandleTypeDef htim6;
    extern TIM_HandleTypeDef htim7;
    extern TIM_HandleTypeDef htim13;

    void MX_TIM6_Init(void);
    void MX_TIM7_Init(void);
    void MX_TIM13_Init(void);

    extern void timer6();       // Обработчик прерывания таймера TIM6	1 раз в 1 милисекунду
    extern void timerMotor_0(); // Обработчик прерывания таймера TIM7
    extern void timerMotor_1(); // Обработчик прерывания таймера TIM13

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim); // Обработчик прерываний для таймеров

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */
