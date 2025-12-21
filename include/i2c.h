
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

extern I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void);

void Scan_I2C_Universal(void); // Запуск умного сканирования

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H__ */

