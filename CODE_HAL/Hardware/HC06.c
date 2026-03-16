#include "main.h" 
#include "usart.h"                
#include <stdio.h>
#include <stdarg.h>

uint8_t Serial_RxData;
uint8_t Serial_RxFlag;


void Serial_SendByte(uint8_t Byte) 
{
    // ����1�ֽ����ݣ�����ģʽ��
    HAL_UART_Transmit(&huart1, &Byte, 1, 100); // ���һ�������ǳ�ʱʱ�䣨ms��
}

void Serial_SendArray(uint8_t *Array,uint16_t Length)   //发送数�?
{
	uint8_t i;
	for(i=0;i<Length;i++)
	{
		Serial_SendByte(Array[i]);
	}
}

void Serial_SendString(char *String)   //发送字�?
{
	uint8_t i;
	for(i=0;String[i]!='\0';i++)
	{
		Serial_SendByte(String[i]);
	}
}

uint32_t Serial_pow(uint32_t X,uint32_t Y)   //X的Y次方计算函数
{
	uint32_t Z=1;
	while(Y--)
	{
		Z *= X;
	}
	return Z;
}

void Serial_SendNumber(uint32_t Number,uint8_t Length)   //发送十进制数字
{
	uint8_t i;
	for(i=0;i<Length;i++)
	{
		Serial_SendByte(Number/Serial_pow(10,Length-i-1)%10+'0');    //从高位到低位依�?�发�?+编码偏移
	}
}

int fputc(int ch,FILE *f)
{
	Serial_SendByte(ch);
	return ch;
}

void Serial_printf(char *format,...)
{
	char String[100];
	va_list arg;
	va_start(arg,format);
	vsprintf(String,format,arg);
	va_end(arg);
	Serial_SendString(String);
}

uint8_t Serial_GetRxFlag(void)
{
	if(Serial_RxFlag == 1)
	{
		Serial_RxFlag = 0;
		return 1;
	}
	return 0;
}


uint8_t Serial_GetRxData(void)
{
	return Serial_RxData;
}






