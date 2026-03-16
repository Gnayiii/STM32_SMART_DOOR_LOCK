#ifndef __M0TOR_H__
#define __M0TOR_H__

#include <stdint.h>

#define IN1(x)			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,(GPIO_PinState)(x));
#define IN2(x)			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_12,(GPIO_PinState)(x));
#define IN3(x)			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_11,(GPIO_PinState)(x));
#define IN4(x)			HAL_GPIO_WritePin(GPIOB,GPIO_PIN_10,(GPIO_PinState)(x));


typedef enum				//控制电机转向（顺时针、逆时针）
{
	cw = 0x00,				//正向
	ccw = 0x01				//逆向
}MotorDirection;

void Motor_HalfDrive(uint8_t step);
void Motor_DirectionSpeed(MotorDirection direction,uint8_t rpm);
void Motor_DirectionAngle90(MotorDirection direction);

#endif

