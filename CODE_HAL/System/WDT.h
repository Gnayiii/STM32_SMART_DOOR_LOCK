#ifndef __WDT_H__
#define __WDT_H__

#include "stm32f1xx_hal.h"

void WDT_Init(void);  /* 独立看门狗初始化 */
void WDT_Feed(void);  /* 喂狗(刷新计数,防超时复位) */

#endif
