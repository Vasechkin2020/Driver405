
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void SPI1_IRQHandler(void);

void TIM6_DAC_IRQHandler(void);
void TIM7_IRQHandler(void);
void TIM8_UP_TIM13_IRQHandler(void);

void DMA2_Stream0_IRQHandler(void);
void DMA2_Stream3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */
