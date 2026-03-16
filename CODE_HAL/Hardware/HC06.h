#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <stdio.h>
#include "main.h"    

void Serial_SendByte(uint16_t Byte);
void Serial_SendArray(uint8_t *Array,uint16_t Length);  
void Serial_SendString(char *String);   
uint32_t Serial_pow(uint32_t X,uint32_t Y);  
void Serial_SendNumber(uint32_t Number,uint8_t Length);   
void Serial_printf(char *format,...);
uint8_t Serial_GetRxFlag(void);
uint8_t Serial_GetRxData(void);

#endif
