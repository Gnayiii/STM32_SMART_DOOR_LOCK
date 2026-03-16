#include "main.h"
#include "Delay.h"

#define Line1			GPIO_PIN_4
#define Line2			GPIO_PIN_5
#define Line3			GPIO_PIN_6
#define Line4			GPIO_PIN_7
#define Column1			GPIO_PIN_3
#define Column2			GPIO_PIN_11
#define Column3			GPIO_PIN_1
#define Column4			GPIO_PIN_0


uint8_t Key_Num;
uint8_t Keyscan;

uint8_t Key_Scan(void)										//按键扫描，获取按下的按键
{
	HAL_GPIO_WritePin(GPIOB, Line1,0);
	HAL_GPIO_WritePin(GPIOB, Line2,1);
	HAL_GPIO_WritePin(GPIOB, Line3,1);
	HAL_GPIO_WritePin(GPIOB, Line4,1);
	if(HAL_GPIO_ReadPin(GPIOB,Column1) == 0)	  return 1;
	else if(HAL_GPIO_ReadPin(GPIOA,Column2) == 0) return 2;
	else if(HAL_GPIO_ReadPin(GPIOB,Column3) == 0) return 3;
	else if(HAL_GPIO_ReadPin(GPIOB,Column4) == 0) return 4;
	
	HAL_GPIO_WritePin(GPIOB, Line1,1);
	HAL_GPIO_WritePin(GPIOB, Line2,0);
	HAL_GPIO_WritePin(GPIOB, Line3,1);
	HAL_GPIO_WritePin(GPIOB, Line4,1);
	if(HAL_GPIO_ReadPin(GPIOB,Column1) == 0) 	  return 5;
	else if(HAL_GPIO_ReadPin(GPIOA,Column2) == 0) return 6;
	else if(HAL_GPIO_ReadPin(GPIOB,Column3) == 0) return 7;
	else if(HAL_GPIO_ReadPin(GPIOB,Column4) == 0) return 8;

	HAL_GPIO_WritePin(GPIOB, Line1,1);
	HAL_GPIO_WritePin(GPIOB, Line2,1);
	HAL_GPIO_WritePin(GPIOB, Line3,0);
	HAL_GPIO_WritePin(GPIOB, Line4,1);
	if(HAL_GPIO_ReadPin(GPIOB,Column1) == 0)	  return 9;
	else if(HAL_GPIO_ReadPin(GPIOA,Column2) == 0) return 10;
	else if(HAL_GPIO_ReadPin(GPIOB,Column3) == 0) return 11;
	else if(HAL_GPIO_ReadPin(GPIOB,Column4) == 0) return 12;

	HAL_GPIO_WritePin(GPIOB, Line1,1);
	HAL_GPIO_WritePin(GPIOB, Line2,1);
	HAL_GPIO_WritePin(GPIOB, Line3,1);
	HAL_GPIO_WritePin(GPIOB, Line4,0);
	if(HAL_GPIO_ReadPin(GPIOB,Column1) == 0) 	  return 13;
	else if(HAL_GPIO_ReadPin(GPIOA,Column2) == 0) return 14;
	else if(HAL_GPIO_ReadPin(GPIOB,Column3) == 0) return 15;
	else if(HAL_GPIO_ReadPin(GPIOB,Column4) == 0) return 16;
	
	return 0;
}

uint8_t Key_GetNum(void) 							//获取按键值，没按键按下则返回0
{
	uint8_t Temp;
	if(Key_Num)
	{
		Temp = Key_Num;
		Key_Num = 0;	
		return Temp;
	}
	return 0;
}

void Key_Tick(void)
{
	static uint8_t count;
	static uint8_t oldState,newState;
	count ++;              							//每次进中断+1
	if(count >= 20) 								//20ms后采样一次
	{
		count = 0;
		oldState = newState; 						//上次采样电平
		newState = Key_Scan();					//这次采样电平
		if(newState == 0 && oldState !=0)			//上次采样为按下，这次采样为没按下，即按键按下松开的瞬间
		{
			Key_Num = oldState;						//按键值则为上次采样的按下的按键
		}
	}
}


