#include "main.h"
#include <stdint.h>
#include "Motor.h"

void Motor_HalfDrive(uint8_t step)
{
	switch(step)
	{
		case 0:
			IN1(1);
			IN2(0);
			IN3(0);
			IN4(0);
		break;
		case 1:
			IN1(1);
			IN2(1);
			IN3(0);
			IN4(0);
		break;
		case 2:
			IN1(0);
			IN2(1);
			IN3(0);
			IN4(0);
		break;
		case 3:
			IN1(0);
			IN2(1);
			IN3(1);
			IN4(0);
		break;
		case 4:
			IN1(0);
			IN2(0);
			IN3(1);
			IN4(0);
		break;
		case 5:
			IN1(0);
			IN2(0);
			IN3(1);
			IN4(1);
		break;
		case 6:
			IN1(0);
			IN2(0);
			IN3(0);
			IN4(1);
		break;
		case 7:
			IN1(1);
			IN2(0);
			IN3(0);
			IN4(1);
		break;
		default:
			break;
	}
}

//1min = 60s = 60 * 1000 ms = 60 * 1000 * 1000 us = 60 000 000 us
//360° / (5.625° / 64) = 4096步		转一圈所需步数
//rpm：每分钟转的圈数
//60 000 000 / 4096 / rpm		间隔时间		
//rpm范围：1-15
void Motor_DirectionSpeed(MotorDirection direction,uint8_t rpm)
{
	int8_t i;
	if(direction == cw)
	{
		for(i=7;i>=0;i--)
		{
			if(rpm > 15) rpm = 15;
			Motor_HalfDrive(i);
			HAL_Delay(60000 / 4096 / rpm);
		}
	}
	else if(direction == ccw)
	{
		for(i=0;i<8;i++)
		{
			if(rpm > 15) rpm = 15;
			Motor_HalfDrive(i);
			HAL_Delay(60000 / 4096 / rpm);
		}
	}
}

//设置方向和角度，角度范围：90,-90
void Motor_DirectionAngle90(MotorDirection direction)
{
	  uint32_t start_time = HAL_GetTick();  // 记录开始时间（毫秒）
   	  uint32_t run_duration = 3000;         // 目标运行时间：3000毫秒（3秒）转半圈
	
    while (HAL_GetTick() - start_time < run_duration)
		{
        Motor_DirectionSpeed(direction, 5);			
		}
}
	
