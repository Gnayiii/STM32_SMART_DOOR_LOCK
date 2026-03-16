#include "main.h"                  

#define BUZZER_PIN                    GPIO_PIN_8
#define BUZZER_PORT                   GPIOB

void Buzzer_ON()
{
	HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN ,GPIO_PIN_RESET);
}

void Buzzer_OFF()
{
	HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN ,GPIO_PIN_SET);
}

void Buzzer_Key()	//按键响声
{
	Buzzer_ON();
	HAL_Delay(50);
	Buzzer_OFF();
}

void Buzzer_Lock()	//解锁关锁响声
{
	Buzzer_ON();
	HAL_Delay(800);
	Buzzer_OFF();
}

void Buzzer_OLED_long()	//长提示音
{
	Buzzer_ON();
	HAL_Delay(400);
	Buzzer_OFF();
}

void Buzzer_OLED_short()	//短提示音
{
	Buzzer_ON();
	HAL_Delay(200);
	Buzzer_OFF();
}

void Buzzer_Alarm()	 //蜂鸣器发出警报
{
	Buzzer_ON();
	HAL_Delay(50);
	Buzzer_OFF();
	HAL_Delay(50);
	Buzzer_ON();
	HAL_Delay(50);
	Buzzer_OFF();
	HAL_Delay(50);
	Buzzer_ON();
	HAL_Delay(50);
	Buzzer_OFF();
	HAL_Delay(50);
	Buzzer_ON();
	HAL_Delay(50);
	Buzzer_OFF();
}
