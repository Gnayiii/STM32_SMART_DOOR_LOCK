#ifndef __FLASH_H__
#define __FLASH_H__

#include "main.h"

//64KB（page52 53 54 55）
#define FLASH_ADDR1 0x0800D000
#define FLASH_ADDR2 0x0800D400
#define FLASH_ADDR3 0x0800D800
#define FLASH_ADDR4 0x0800DC00

void FLASH_W(uint32_t add,uint8_t dat1,uint8_t dat2,uint8_t dat3,uint8_t dat4);
uint16_t FLASH_R(uint32_t add);
void FLASH_Clear(uint32_t add);

#endif

