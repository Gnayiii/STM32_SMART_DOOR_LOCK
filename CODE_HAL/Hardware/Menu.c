#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "Delay.h"
#include "Key.h"
#include "Menu.h"
#include <string.h>
#include "Motor.h"
#include "PasswordFlash.h"
#include "MFRC522.h"
#include "Flash.h"
#include "BUZZER.h"
#include "AS608.h"
#include "FingerprintFlash.h"
#include "Security.h"
#include "WDT.h"

void RFID_Check(void);
void Read_Card(void);

extern uint8_t UID[4], Temp[4];
extern uint8_t UI0[4]; // 锟斤拷片0ID锟斤拷锟斤拷
extern uint8_t UI1[4]; // 锟斤拷片1ID锟斤拷锟斤拷
extern uint8_t UI2[4]; // 锟斤拷片2ID锟斤拷锟斤拷
extern uint8_t UI3[4]; // 锟斤拷片3ID锟斤拷锟斤拷

uint8_t KeyNum;

/*-----------------------------------------进入系统-------------------------------------------*/

void Show_Emoji_UI(void)
{
	// 闭眼
	for (uint8_t i = 0; i < 3; i++)
	{
		OLED_Clear();
		OLED_ShowImage(30, 10 + i, 16, 16, Eyebrow1); // 左眉毛
		OLED_ShowImage(82, 10 + i, 16, 16, Eyebrow2); // 右眉毛
		OLED_DrawEllipse(40, 32, 6, 6 - i, 1);		  // 左眼
		OLED_DrawEllipse(88, 32, 6, 6 - i, 1);		  // 右眼
		OLED_ShowImage(54, 40, 20, 20, Mouth);		  // 嘴巴
		OLED_Update();
		HAL_Delay(100);
	}
	// 睁眼
	for (uint8_t i = 0; i < 3; i++)
	{
		OLED_Clear();
		OLED_ShowImage(30, 12 - i, 16, 16, Eyebrow1); // 左眉毛
		OLED_ShowImage(82, 12 - i, 16, 16, Eyebrow2); // 右眉毛
		OLED_DrawEllipse(40, 32, 6, 4 + i, 1);		  // 左眼
		OLED_DrawEllipse(88, 32, 6, 4 + i, 1);		  // 右眼
		OLED_ShowImage(54, 40, 20, 20, Mouth);		  // 嘴巴
		OLED_Update();
		HAL_Delay(100);
	}
	HAL_Delay(500);
}

void Menu_Show_UI(void)
{
	OLED_Clear();
	OLED_ShowString(16, 24, "智能门禁系统", OLED_8X16);
	OLED_Update();
	HAL_Delay(1000);
	for (uint8_t i = 0; i < 2; i++)
	{
		Show_Emoji_UI();
	}
}

/*-----------------------------------------关锁界面-------------------------------------------*/
void Lock_UI(void)
{
	OLED_Clear();
	OLED_ShowImage(50, 10, 24, 24, Lock);
	OLED_ShowString(32, 40, "密码解锁", OLED_8X16);
	OLED_ReverseArea(32, 40, 64, 16);
	OLED_Update();
}

void Lock_Page(void)
{
	if (Security_IsLocked()) // 反暴力:锁定期间显示倒计时并忽略按键
	{
		uint32_t sec = (Security_LockRemainMs() + 999) / 1000; //+999向上取整
		OLED_Clear();
		OLED_ShowString(24, 20, "安全锁定", OLED_8X16);
		OLED_ShowString(8, 40, "剩余", OLED_8X16);
		OLED_ShowNum(56, 40, sec, 2, OLED_8X16);
		OLED_ShowString(88, 40, "秒", OLED_8X16);
		OLED_Update();
		(void)Key_GetNum(); // 调用一次将Key_Num值清空，且将返回值丢弃(void)，否则锁定结束会读取Key_Num值造成误操作
		return;
	}
	Lock_UI();
	KeyNum = Key_GetNum();
	if (KeyNum)
	{
		UnLock_Page();
	}
}

/*-----------------------------------------菜单界面-------------------------------------------*/

void Menu_UI(void)
{
	OLED_Clear();
	OLED_ShowString(0, 0, "一、修改密码", OLED_8X16);
	OLED_ShowString(0, 16, "二、录入新卡", OLED_8X16);
	OLED_ShowString(0, 32, "三、删除卡片", OLED_8X16);
	OLED_ShowString(0, 48, "四、关锁", OLED_8X16);
}

void Menu_UI_Next(void)
{
	OLED_Clear();
	OLED_ShowString(0, 0, "五、录入指纹", OLED_8X16);
	OLED_ShowString(0, 16, "六、删除指纹", OLED_8X16);
	OLED_ShowString(0, 32, "七、开锁日志", OLED_8X16);
}

uint8_t Flag = 1;

uint8_t First_Page(void)
{
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		KeyNum = Key_GetNum();
		if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
		switch (KeyNum)
		{
		case 13: // 上一项
			Flag--;
			if (Flag <= 0)
				Flag = 7;
			break;
		case 14: // 下一项
			Flag++;
			if (Flag >= 8)
				Flag = 1;
			break;
		case 16: // 确认
			if (Flag == 4)
				Buzzer_Lock();
			else
				Buzzer_Key();
			OLED_Clear();
			OLED_Update();
			return Flag;
		}

		switch (Flag)
		{
		case 1:
			Menu_UI();
			OLED_ReverseArea(0, 0, 96, 16);
			OLED_Update();
			break;
		case 2:
			Menu_UI();
			OLED_ReverseArea(0, 16, 96, 16);
			OLED_Update();
			break;
		case 3:
			Menu_UI();
			OLED_ReverseArea(0, 32, 96, 16);
			OLED_Update();
			break;
		case 4:
			Menu_UI();
			OLED_ReverseArea(0, 48, 64, 16);
			OLED_Update();
			break;
		case 5:
			Menu_UI_Next();
			OLED_ReverseArea(0, 0, 96, 16);
			OLED_Update();
			break;
		case 6:
			Menu_UI_Next();
			OLED_ReverseArea(0, 16, 96, 16);
			OLED_Update();
			break;
		case 7:
			Menu_UI_Next();
			OLED_ReverseArea(0, 32, 96, 16);
			OLED_Update();
			break;
		}
	}
}

/*-----------------------------------------密码解锁-------------------------------------------*/

char input_password[PASSWORD_LEN + 1] = {0}; // 存储输入的密码
int input_pos = 0;							 // 当前输入位置
uint8_t unlock_flag = 0;
uint8_t LockFlag = 0;

// 显示输入密码函数
void Display_Password(void)
{
	OLED_ShowString(16, 0, "输入四位密码", OLED_8X16);
	OLED_ShowImage(4, 28, 14, 16, Boxleft);
	OLED_ShowImage(110, 28, 14, 16, Boxright);
	OLED_Update();
	for (uint8_t i = 0; i < PASSWORD_LEN; i++)
	{
		if (i < input_pos)
		{
			OLED_ShowChar(30 + i * 20, 28, ' ', OLED_8X16);
			OLED_ShowChar(30 + i * 20, 28, '*', OLED_8X16); // 已输入的位显示为*
			OLED_Update();
		}
		else
		{
			OLED_ShowChar(30 + i * 20, 28, ' ', OLED_8X16);
			OLED_ShowChar(30 + i * 20, 28, '_', OLED_8X16); // 未输入的位显示为_
			OLED_Update();
		}
	}
}

/**
 * @brief  验证密码是否正确（从Flash读取密码进行比对）
 * @retval 1：密码正确；0：密码错误
 */
uint8_t Check_Password(void)
{
	char stored_pass[PASSWORD_LEN + 1];
	// 从Flash读取密码，若读取失败则默认使用默认密码"1106"兼容
	if (PasswordFlash_Read(stored_pass) != 1)
	{
		return strcmp(input_password, Password_Default) == 0;
	}
	// 比对输入密码和存储密码
	return strcmp(input_password, stored_pass) == 0;
}

void Intput_Password(uint8_t keyflag)
{
	// 数字键处理（0-9）
	uint8_t key;
	if (keyflag == 10)
		key = 0;
	else if (keyflag == 0)
		key = 10;
	else
		key = keyflag;
	if (key <= 9)
	{
		if (input_pos < PASSWORD_LEN)
		{
			input_password[input_pos++] = '0' + key; // 转换为字符
			input_password[input_pos] = '\0';		 // 确保字符串结束
		}
	}
	// 删除键处理
	else if (key == 15)
	{
		if (input_pos > 0)
		{
			input_pos--;
			input_password[input_pos] = '\0'; // 清除最后一位
		}
	}
	// 确认键处理
	else if (key == 16)
	{
		if (input_pos == PASSWORD_LEN)
		{ // 确保输入了4位
			if (Check_Password())
			{
				OLED_Clear();
				OLED_ShowString(28, 28, "解锁成功！", OLED_8X16);
				OLED_Update();
				Buzzer_Lock();
				Motor_DirectionAngle90(ccw);
				OLED_Clear();
				Flag = 1;
				LockFlag = 1;
				unlock_flag = 1;
				Security_OnAuthSuccess(CH_KEYPAD); /* 键盘认证成功 */
												   // 解锁操作
			}
			else
			{
				Security_OnAuthFail(CH_KEYPAD); /* 键盘认证失败 */
				OLED_Clear();
				OLED_ShowString(28, 28, "密码错误！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_long();
				HAL_Delay(600);
				OLED_Clear();
				// 清空输入，重新开始
				for (int i = 0; i < sizeof(input_password); i++)
				{
					input_password[i] = 0; // 0 等价于 '\0'
				}
				input_pos = 0; // 同时重置输入位置
			}
		}
		else
		{
			OLED_ShowString(16, 48, "密码不足四位", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_short();
			HAL_Delay(600);
			OLED_Clear();
		}
	}

	// 更新显示
	Display_Password();
}

/* 自动回锁执行:到期关锁并回锁屏;Menu.c 拥有 LockFlag */
uint8_t AutoRelock_CheckAndExec(void)
{
	if (Security_RelockDue())
	{
		OLED_Clear();
		OLED_ShowString(20, 28, "自动关锁", OLED_8X16);
		OLED_Update();
		Buzzer_Lock();
		Motor_DirectionAngle90(cw); /* 关锁 */
		Flag = 1;
		LockFlag = 0; /* 回锁屏 */
		return 1;
	}
	return 0;
}

/* 有界等待卡片移开:替代 MFRC522 的无限 WaitCardOff(看门狗集成适配) */
uint8_t WaitCardOff_Timeout(uint32_t timeout_ms)
{
	uint32_t t0 = HAL_GetTick();
	unsigned char TagType[2];
	while ((HAL_GetTick() - t0) < timeout_ms)
	{
		WDT_Feed(); /* 看门狗喂狗 */
		if (PcdRequest(REQ_ALL, TagType) != MI_OK &&
			PcdRequest(REQ_ALL, TagType) != MI_OK &&
			PcdRequest(REQ_ALL, TagType) != MI_OK) return 1; /* 卡已移开 */
		delay_10ms(10);
	}
	return 0; /* 超时,卡仍在 */
}

uint8_t UnLock_Page(void)
{
	OLED_Clear();
	for (int i = 0; i < sizeof(input_password); i++)
	{
		input_password[i] = 0; // 0 等价于 '\0'
	}
	input_pos = 0; // 同时重置输入位置
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		if (Security_IsLocked())
			return 0; /* 反暴力:锁定后立即退出密码输入,回锁屏显示倒计时 */
		if (unlock_flag == 0)
		{
			KeyNum = Key_GetNum();
			if (KeyNum == 13)
				return 0;
			Intput_Password(KeyNum);
		}
		else
		{
			unlock_flag = 0;
			return 0;
		}
	}
}

/*-----------------------------------------修改密码界面-------------------------------------------*/

// 显示输入初始密码函数
void Display_OldPassword(void)
{
	OLED_ShowString(16, 0, "输入初始密码", OLED_8X16);
	OLED_ShowImage(4, 28, 14, 16, Boxleft);
	OLED_ShowImage(110, 28, 14, 16, Boxright);
	OLED_Update();
	for (uint8_t i = 0; i < PASSWORD_LEN; i++)
	{
		if (i < input_pos)
		{
			OLED_ShowChar(30 + i * 20, 28, ' ', OLED_8X16);
			OLED_ShowChar(30 + i * 20, 28, '*', OLED_8X16); // 已输入的位显示为*
			OLED_Update();
		}
		else
		{
			OLED_ShowChar(30 + i * 20, 28, ' ', OLED_8X16);
			OLED_ShowChar(30 + i * 20, 28, '_', OLED_8X16); // 未输入的位显示为_
			OLED_Update();
		}
	}
}

// 显示输入新密码函数
void Display_NewPassword(void)
{
	OLED_ShowString(16, 0, "请输入新密码", OLED_8X16);
	OLED_ShowImage(4, 28, 14, 16, Boxleft);
	OLED_ShowImage(110, 28, 14, 16, Boxright);
	OLED_Update();
	for (uint8_t i = 0; i < PASSWORD_LEN; i++)
	{
		if (i < input_pos)
		{
			OLED_ShowChar(30 + i * 20, 28, ' ', OLED_8X16);
			OLED_ShowChar(30 + i * 20, 28, '*', OLED_8X16); // 已输入的位显示为*
			OLED_Update();
		}
		else
		{
			OLED_ShowChar(30 + i * 20, 28, ' ', OLED_8X16);
			OLED_ShowChar(30 + i * 20, 28, '_', OLED_8X16); // 未输入的位显示为_
			OLED_Update();
		}
	}
}

void Intput_OldPassword(uint8_t keyflag)
{
	// 数字键处理（0-9）
	uint8_t key;
	if (keyflag == 10)
		key = 0;
	else if (keyflag == 0)
		key = 10;
	else
		key = keyflag;
	if (key <= 9)
	{
		if (input_pos < PASSWORD_LEN)
		{
			input_password[input_pos++] = '0' + key; // 转换为字符
			input_password[input_pos] = '\0';		 // 确保字符串结束
		}
	}
	// 删除键处理
	else if (key == 15)
	{
		if (input_pos > 0)
		{
			input_pos--;
			input_password[input_pos] = '\0'; // 清除最后一位
		}
	}
	// 确认键处理
	else if (key == 16)
	{
		if (input_pos == PASSWORD_LEN)
		{ // 确保输入了4位
			if (Check_Password())
			{
				Buzzer_OLED_short();
				OLED_Clear();
				NewPassword_Page();
				// 跳转到输入新密码界面
			}
			else
			{
				OLED_Clear();
				OLED_ShowString(28, 28, "密码错误！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_long();
				HAL_Delay(600);
				OLED_Clear();
				// 清空输入，重新开始
				for (int i = 0; i < sizeof(input_password); i++)
				{
					input_password[i] = 0; // 0 等价于 '\0'
				}
				input_pos = 0; // 同时重置输入位置
			}
		}
		else
		{
			OLED_ShowString(16, 48, "密码不足四位", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_short();
			HAL_Delay(600);
			OLED_Clear();
		}
	}

	// 更新显示
	Display_OldPassword();
}

static uint8_t intpassword_flag = 0;
static uint8_t newpassword_flag = 0;

void Intput_NewPassword(uint8_t keyflag)
{
	// 数字键处理（0-9）
	uint8_t key;
	if (keyflag == 10)
		key = 0;
	else if (keyflag == 0)
		key = 10;
	else
		key = keyflag;
	if (key <= 9)
	{
		if (input_pos < PASSWORD_LEN)
		{
			input_password[input_pos++] = '0' + key; // 转换为字符
			input_password[input_pos] = '\0';		 // 确保字符串结束
		}
	}
	// 删除键处理
	else if (key == 15)
	{
		if (input_pos > 0)
		{
			input_pos--;
			input_password[input_pos] = '\0'; // 清除最后一位
		}
	}
	// 确认键处理
	else if (key == 16)
	{
		if (input_pos == PASSWORD_LEN)
		{ // 确保输入了4位
			OLED_Clear();
			OLED_ShowString(28, 28, "修改成功！", OLED_8X16);
			OLED_Update();
			PasswordFlash_Write(input_password);
			Buzzer_OLED_long();
			HAL_Delay(400);
			OLED_Clear();
			intpassword_flag = 1;
			newpassword_flag = 1;
			// 跳转到输入新密码界面
		}
		else
		{
			OLED_ShowString(16, 48, "密码不足四位", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_short();
			HAL_Delay(600);
			OLED_Clear();
		}
	}

	// 更新显示
	Display_NewPassword();
}

uint8_t OldPassword_Page(void)
{
	for (int i = 0; i < sizeof(input_password); i++)
	{
		input_password[i] = 0; // 0 等价于 '\0'
	}
	input_pos = 0; // 同时重置输入位置
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		if (intpassword_flag == 0)
		{
			KeyNum = Key_GetNum();
			if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
			Intput_OldPassword(KeyNum);
			if (KeyNum == 13)
				return 0;
		}
		else
		{
			intpassword_flag = 0;
			return 0;
		}
	}
}

uint8_t NewPassword_Page(void)
{
	for (int i = 0; i < sizeof(input_password); i++)
	{
		input_password[i] = 0; // 0 等价于 '\0'
	}
	input_pos = 0; // 同时重置输入位置
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		if (newpassword_flag == 0)
		{
			KeyNum = Key_GetNum();
			if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
			Intput_NewPassword(KeyNum);
			if (KeyNum == 13)
				return 0;
		}
		else
		{
			newpassword_flag = 0;
			return 0;
		}
	}
}

/*-----------------------------------------录入新卡-------------------------------------------*/

uint8_t tempcard, select = 0;

void Add_Card_UI(void)
{
	Read_Card();
	if (UI0[0] == 0xFF && UI0[1] == 0xFF && UI0[2] == 0xFF && UI0[3] == 0xFF && // 没有卡的数据，请录入新卡
		UI1[0] == 0xFF && UI1[1] == 0xFF && UI1[2] == 0xFF && UI1[3] == 0xFF &&
		UI2[0] == 0xFF && UI2[1] == 0xFF && UI2[2] == 0xFF && UI2[3] == 0xFF &&
		UI3[0] == 0xFF && UI3[1] == 0xFF && UI3[2] == 0xFF && UI3[3] == 0xFF)
	{
		OLED_Clear();
		OLED_ShowString(24, 28, "请录入新卡！", OLED_8X16);
		OLED_Update();
	}
	else
	{
		OLED_Clear();
		OLED_ShowString(0, 0, "卡一", OLED_8X16);
		OLED_ShowChar(32, 0, ':', OLED_8X16);
		OLED_ShowString(0, 16, "卡二", OLED_8X16);
		OLED_ShowChar(32, 16, ':', OLED_8X16);
		OLED_ShowString(0, 32, "卡三", OLED_8X16);
		OLED_ShowChar(32, 32, ':', OLED_8X16);
		OLED_ShowString(0, 48, "卡四", OLED_8X16);
		OLED_ShowChar(32, 48, ':', OLED_8X16);
		for (uint8_t i = 0; i < 4; i++)
		{
			OLED_Printf(40 + i * 16, 0, OLED_8X16, "%02X", UI0[i]);
			OLED_Printf(40 + i * 16, 16, OLED_8X16, "%02X", UI1[i]);
			OLED_Printf(40 + i * 16, 32, OLED_8X16, "%02X", UI2[i]);
			OLED_Printf(40 + i * 16, 48, OLED_8X16, "%02X", UI3[i]);
		}
		if (UI0[0] == 0xFF && UI0[1] == 0xFF && UI0[2] == 0xFF && UI0[3] == 0xFF)
			OLED_ShowImage(0, 0, 128, 16, Empty);
		if (UI1[0] == 0xFF && UI1[1] == 0xFF && UI1[2] == 0xFF && UI1[3] == 0xFF)
			OLED_ShowImage(0, 16, 128, 16, Empty);
		if (UI2[0] == 0xFF && UI2[1] == 0xFF && UI2[2] == 0xFF && UI2[3] == 0xFF)
			OLED_ShowImage(0, 32, 128, 16, Empty);
		if (UI3[0] == 0xFF && UI3[1] == 0xFF && UI3[2] == 0xFF && UI3[3] == 0xFF)
			OLED_ShowImage(0, 48, 128, 16, Empty);
		OLED_Update();
	}
}

void Add_Card(void)
{
	if (PcdRequest(REQ_ALL, Temp) == MI_OK)
	{
		if (PcdAnticoll(UID) == MI_OK)
		{
			if (UI0[0] == 0xFF && UI0[1] == 0xFF && UI0[2] == 0xFF && UI0[3] == 0xFF)
				tempcard = 0; // 判断系统各卡数据区是否为空，为空才能写入新卡
			else if (UI1[0] == 0xFF && UI1[1] == 0xFF && UI1[2] == 0xFF && UI1[3] == 0xFF)
				tempcard = 1;
			else if (UI2[0] == 0xFF && UI2[1] == 0xFF && UI2[2] == 0xFF && UI2[3] == 0xFF)
				tempcard = 2;
			else if (UI3[0] == 0xFF && UI3[1] == 0xFF && UI3[2] == 0xFF && UI3[3] == 0xFF)
				tempcard = 3;
			else
				tempcard = 4;

			if (UID[0] == UI0[0] && UID[1] == UI0[1] && UID[2] == UI0[2] && UID[3] == UI0[3]) // 判断新卡是否已经录入
			{
				tempcard = 5;
			}
			if (UID[0] == UI1[0] && UID[1] == UI1[1] && UID[2] == UI1[2] && UID[3] == UI1[3])
			{
				tempcard = 5;
			}
			if (UID[0] == UI2[0] && UID[1] == UI2[1] && UID[2] == UI2[2] && UID[3] == UI2[3])
			{
				tempcard = 5;
			}
			if (UID[0] == UI3[0] && UID[1] == UI3[1] && UID[2] == UI3[2] && UID[3] == UI3[3])
			{
				tempcard = 5;
			}

			switch (tempcard)
			{
			case 0:
			{
				UI0[0] = UID[0]; // 将新卡数据写入UI0[]数组
				UI0[1] = UID[1];
				UI0[2] = UID[2];
				UI0[3] = UID[3];
				FLASH_W(FLASH_ADDR1, UI0[0], UI0[1], UI0[2], UI0[3]); // 将新卡数据存入flash
				OLED_Clear();										  // 写卡成功，蜂鸣器响一声
				OLED_ShowString(16, 28, "录入卡一成功！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_long();
				WaitCardOff_Timeout(5000); // 等待卡片移走
			}
			break;
			case 1:
			{
				UI1[0] = UID[0];
				UI1[1] = UID[1];
				UI1[2] = UID[2];
				UI1[3] = UID[3];
				FLASH_W(FLASH_ADDR2, UI1[0], UI1[1], UI1[2], UI1[3]);
				OLED_Clear();
				OLED_ShowString(16, 28, "录入卡二成功！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_long();
				WaitCardOff_Timeout(5000);
			}
			break;
			case 2:
			{
				UI2[0] = UID[0];
				UI2[1] = UID[1];
				UI2[2] = UID[2];
				UI2[3] = UID[3];
				FLASH_W(FLASH_ADDR3, UI2[0], UI2[1], UI2[2], UI2[3]);
				OLED_Clear();
				OLED_ShowString(16, 28, "录入卡三成功！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_long();
				WaitCardOff_Timeout(5000);
			}
			break;
			case 3:
			{
				UI3[0] = UID[0];
				UI3[1] = UID[1];
				UI3[2] = UID[2];
				UI3[3] = UID[3];
				FLASH_W(FLASH_ADDR4, UI3[0], UI3[1], UI3[2], UI3[3]);
				OLED_Clear();
				OLED_ShowString(16, 28, "录入卡四成功！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_long();
				WaitCardOff_Timeout(5000);
			}
			break;
			case 4:
			{
				OLED_Clear(); // 若4个存卡数组均已存入卡片则显示无数据空间，蜂鸣器发出警报
				OLED_ShowString(28, 28, "卡位已满！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_short();
				WaitCardOff_Timeout(5000);
			}
			case 5:
			{
				OLED_Clear(); // 若新卡已录入，蜂鸣器发出警报
				OLED_ShowString(24, 28, "新卡已存在！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_short();
				WaitCardOff_Timeout(5000);
			}
			default:
				break;
			}
		}
	}
}

uint8_t Add_Card_Page(void)
{
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		KeyNum = Key_GetNum();
		if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
		Add_Card_UI();
		Add_Card();
		if (KeyNum)
			return 0;
	}
}

/*-----------------------------------------删除卡片-------------------------------------------*/

void Delete_Card_UI(void)
{
	Read_Card();
	OLED_ShowString(0, 0, "卡一", OLED_8X16);
	OLED_ShowChar(32, 0, ':', OLED_8X16);
	OLED_ShowString(0, 16, "卡二", OLED_8X16);
	OLED_ShowChar(32, 16, ':', OLED_8X16);
	OLED_ShowString(0, 32, "卡三", OLED_8X16);
	OLED_ShowChar(32, 32, ':', OLED_8X16);
	OLED_ShowString(0, 48, "卡四", OLED_8X16);
	OLED_ShowChar(32, 48, ':', OLED_8X16);
	for (uint8_t i = 0; i < 4; i++)
	{
		OLED_Printf(40 + i * 16, 0, OLED_8X16, "%02X", UI0[i]);
		OLED_Printf(40 + i * 16, 16, OLED_8X16, "%02X", UI1[i]);
		OLED_Printf(40 + i * 16, 32, OLED_8X16, "%02X", UI2[i]);
		OLED_Printf(40 + i * 16, 48, OLED_8X16, "%02X", UI3[i]);
	}
	if (UI0[0] == 0xFF && UI0[1] == 0xFF && UI0[2] == 0xFF && UI0[3] == 0xFF)
		OLED_ShowImage(0, 0, 128, 16, Empty);
	if (UI1[0] == 0xFF && UI1[1] == 0xFF && UI1[2] == 0xFF && UI1[3] == 0xFF)
		OLED_ShowImage(0, 16, 128, 16, Empty);
	if (UI2[0] == 0xFF && UI2[1] == 0xFF && UI2[2] == 0xFF && UI2[3] == 0xFF)
		OLED_ShowImage(0, 32, 128, 16, Empty);
	if (UI3[0] == 0xFF && UI3[1] == 0xFF && UI3[2] == 0xFF && UI3[3] == 0xFF)
		OLED_ShowImage(0, 48, 128, 16, Empty);
}

void Delete_Card(uint8_t select)
{
	switch (select)
	{
	case 1:
		if (UI0[0] == 0xFF && UI0[1] == 0xFF && UI0[2] == 0xFF && UI0[3] == 0xFF)
		{
			OLED_Clear();
			OLED_ShowString(28, 28, "无效删除！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_short();
			HAL_Delay(400);
		}
		else
		{
			FLASH_Clear(FLASH_ADDR1);
			UI0[0] = 0xFF;
			UI0[1] = 0xFF;
			UI0[2] = 0xFF;
			UI0[3] = 0xFF;
			OLED_Clear();
			OLED_ShowString(28, 28, "删除成功！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_long();
			HAL_Delay(600);
		}
		break;
	case 2:
		if (UI1[0] == 0xFF && UI1[1] == 0xFF && UI1[2] == 0xFF && UI1[3] == 0xFF)
		{
			OLED_Clear();
			OLED_ShowString(28, 28, "无效删除！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_short();
			HAL_Delay(400);
		}
		else
		{
			FLASH_Clear(FLASH_ADDR2);
			UI1[0] = 0xFF;
			UI1[1] = 0xFF;
			UI1[2] = 0xFF;
			UI1[3] = 0xFF;
			OLED_Clear();
			OLED_ShowString(28, 28, "删除成功！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_long();
			HAL_Delay(600);
		}
		break;
	case 3:
		if (UI2[0] == 0xFF && UI2[1] == 0xFF && UI2[2] == 0xFF && UI2[3] == 0xFF)
		{
			OLED_Clear();
			OLED_ShowString(28, 28, "无效删除！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_short();
			HAL_Delay(400);
		}
		else
		{
			FLASH_Clear(FLASH_ADDR3);
			UI2[0] = 0xFF;
			UI2[1] = 0xFF;
			UI2[2] = 0xFF;
			UI2[3] = 0xFF;
			OLED_Clear();
			OLED_ShowString(28, 28, "删除成功！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_long();
			HAL_Delay(600);
		}
		break;
	case 4:
		if (UI3[0] == 0xFF && UI3[1] == 0xFF && UI3[2] == 0xFF && UI3[3] == 0xFF)
		{
			OLED_Clear();
			OLED_ShowString(28, 28, "无效删除！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_short();
			HAL_Delay(400);
		}
		else
		{
			FLASH_Clear(FLASH_ADDR4);
			UI3[0] = 0xFF;
			UI3[1] = 0xFF;
			UI3[2] = 0xFF;
			UI3[3] = 0xFF;
			OLED_Clear();
			OLED_ShowString(28, 28, "删除成功！", OLED_8X16);
			OLED_Update();
			Buzzer_OLED_long();
			HAL_Delay(600);
		}
		break;
	default:
		break;
	}
}

uint8_t deleteflag = 1;

uint8_t Delete_Card_Page(void)
{
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		KeyNum = Key_GetNum();
		if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
		select = 0;
		if ((KeyNum >= 1 && KeyNum <= 12) | KeyNum == 15)
		{
			return 0;
		}

		switch (KeyNum)
		{
		case 13: // 上一项
			deleteflag--;
			if (deleteflag <= 0)
				deleteflag = 4;
			break;
		case 14: // 下一项
			deleteflag++;
			if (deleteflag >= 5)
				deleteflag = 1;
			break;
		case 16: // 确认
			OLED_Clear();
			OLED_Update();
			select = deleteflag;
			break;
		}
		switch (deleteflag)
		{
		case 1:
			Delete_Card_UI();
			OLED_ReverseArea(0, 0, 104, 16);
			OLED_Update();
			break;
		case 2:
			Delete_Card_UI();
			OLED_ReverseArea(0, 16, 104, 16);
			OLED_Update();
			break;
		case 3:
			Delete_Card_UI();
			OLED_ReverseArea(0, 32, 104, 16);
			OLED_Update();
			break;
		case 4:
			Delete_Card_UI();
			OLED_ReverseArea(0, 48, 104, 16);
			OLED_Update();
			break;
		}
		Delete_Card(select);
	}
}

/*-----------------------------------------录入指纹-------------------------------------------*/

// 检查 flash 中是否包含指定 slot（1..4）
static uint8_t Flash_HasSlot(uint8_t slot)
{
	uint16_t arr[FINGERPRINT_SLOT_COUNT];
	uint8_t cnt = FingerprintFlash_ReadAll(arr, FINGERPRINT_SLOT_COUNT);
	for (uint8_t i = 0; i < cnt; i++)
	{
		if ((uint16_t)slot == arr[i])
			return 1;
	}
	return 0;
}

/* 更新显示：从 flash 读取并显示最多4个ID */
// UI：从 flash 读取并显示最多4个编号（格式 0001..0004 或空）
void Add_Fingerprint_UI(void)
{
	uint16_t arr[FINGERPRINT_SLOT_COUNT];
	for (uint8_t i = 0; i < FINGERPRINT_SLOT_COUNT; i++)
		arr[i] = 0xFFFF;
	FingerprintFlash_ReadAll(arr, FINGERPRINT_SLOT_COUNT);

	OLED_Clear();
	OLED_ShowString(0, 0, "指纹一", OLED_8X16);
	OLED_ShowChar(48, 0, ':', OLED_8X16);
	OLED_ShowString(0, 16, "指纹二", OLED_8X16);
	OLED_ShowChar(48, 16, ':', OLED_8X16);
	OLED_ShowString(0, 32, "指纹三", OLED_8X16);
	OLED_ShowChar(48, 32, ':', OLED_8X16);
	OLED_ShowString(0, 48, "指纹四", OLED_8X16);
	OLED_ShowChar(48, 48, ':', OLED_8X16);

	// 对于每个逻辑编号 1..4，若在 flash 中存在则显示编号 0001..0004，否则显示空图（用 Empty）
	for (uint8_t slot = 1; slot <= FINGERPRINT_SLOT_COUNT; slot++)
	{
		if (Flash_HasSlot(slot))
		{
			// 格式化为4位字符串 "0001".."0004"
			char buf[5]; // 前3位为0，最后一位为编号字符，末尾为字符串结束符
			buf[0] = '0';
			buf[1] = '0';
			buf[2] = '0';
			buf[3] = '0' + (slot % 10);
			buf[4] = '\0';
			// 在对应行显示编号（横坐标可根据需要调整）
			OLED_ShowString(56, 16 * (slot - 1), buf, OLED_8X16);
		}
		else
		{
			// 空位显示图标（与 Add_Card_UI 一致）
			OLED_ShowImage(0, 16 * (slot - 1), 128, 16, Empty);
		}
	}

	uint8_t curCount = FingerprintFlash_Count();
	if (curCount >= FINGERPRINT_SLOT_COUNT) // 指纹已满4个
	{
		OLED_Clear();
		OLED_ShowString(12, 28, "指纹已满四个！", OLED_8X16);
	}
	else if (curCount == 0) // 指纹为空
	{
		OLED_Clear();
		OLED_ShowString(20, 28, "请录入指纹！", OLED_8X16);
	}

	OLED_Update();
}

uint8_t Add_Fingerprint(uint32_t timeout_ms)
{
	// 检查是否已满，满则禁用录入指纹功能
	uint8_t curCount = FingerprintFlash_Count();
	if (curCount >= FINGERPRINT_SLOT_COUNT)
		return 0;

	// 找第一个空槽 slot (1..4)
	uint8_t slot = 0;
	for (uint8_t s = 1; s <= FINGERPRINT_SLOT_COUNT; s++)
	{
		if (!Flash_HasSlot(s))
		{
			slot = s;
			break;
		}
	}
	if (slot == 0)
		return 0;

	uint16_t pageID = (uint16_t)(slot - 1); // 约定 pageID = slot-1（0001->page 0）

	// 执行录入流程（AS608_Add 已处理等待/超时逻辑）
	if (AS608_Add(pageID, timeout_ms) == 0)
		return 0;

	// 录入成功后把 slot 写入 flash（去重在底层处理）
	if (FingerprintFlash_AddSlot(slot) == 0)
	{
		// 写 flash 失败（返回失败）；模块内已录入但 flash 记录失败
		return 0;
	}

	return 1;
}

uint8_t Add_Fingerprint_Page(void)
{
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		KeyNum = Key_GetNum();
		if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
		Add_Fingerprint_UI();
		if (PS_ReadTouch())
		{
			// 等待稳定（简单去抖）
			HAL_Delay(30);
			if (!PS_ReadTouch())
			{
				HAL_Delay(80);
				continue;
			}

			OLED_Clear();
			OLED_ShowString(20, 28, "正在录入", OLED_8X16);
			OLED_ShowChar(84, 28, '.', OLED_8X16);
			OLED_ShowChar(92, 28, '.', OLED_8X16);
			OLED_ShowChar(100, 28, '.', OLED_8X16);
			OLED_Update();

			// 调用录入函数（阻塞直到超时或完成）
			if (Add_Fingerprint(10000))
			{
				OLED_Clear();
				OLED_ShowString(28, 28, "录入成功！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_long();
				HAL_Delay(600);
			}
			else
			{
				OLED_Clear();
				OLED_ShowString(28, 28, "录入失败！", OLED_8X16);
				OLED_Update();
				Buzzer_OLED_short();
				HAL_Delay(400);
			}

			// 等待手指移开再继续（避免重复触发）
			uint32_t t = HAL_GetTick();             /* 看门狗:死等改有界+喂狗 */
			while (PS_ReadTouch() && (HAL_GetTick() - t) < 5000) { WDT_Feed(); HAL_Delay(50); }

			// 小延时后刷新 UI
			HAL_Delay(200);
		}

		HAL_Delay(80);
		if (KeyNum)
			return 0;
	}
}

/*-----------------------------------------删除指纹-------------------------------------------*/

void Delete_Fingerprint_UI(void)
{
	uint16_t arr[FINGERPRINT_SLOT_COUNT];
	for (uint8_t i = 0; i < FINGERPRINT_SLOT_COUNT; i++)
		arr[i] = 0xFFFF;
	FingerprintFlash_ReadAll(arr, FINGERPRINT_SLOT_COUNT);

	OLED_Clear();
	OLED_ShowString(0, 0, "指纹一", OLED_8X16);
	OLED_ShowChar(48, 0, ':', OLED_8X16);
	OLED_ShowString(0, 16, "指纹二", OLED_8X16);
	OLED_ShowChar(48, 16, ':', OLED_8X16);
	OLED_ShowString(0, 32, "指纹三", OLED_8X16);
	OLED_ShowChar(48, 32, ':', OLED_8X16);
	OLED_ShowString(0, 48, "指纹四", OLED_8X16);
	OLED_ShowChar(48, 48, ':', OLED_8X16);

	for (uint8_t slot = 1; slot <= FINGERPRINT_SLOT_COUNT; slot++)
	{
		if (Flash_HasSlot(slot))
		{
			// 显示编号 0001..0004（格式化）
			char buf[5];
			buf[0] = '0';
			buf[1] = '0';
			buf[2] = '0';
			buf[3] = '0' + (slot % 10);
			buf[4] = '\0';
			OLED_ShowString(56, 16 * (slot - 1), buf, OLED_8X16);
		}
		else
		{
			OLED_ShowImage(0, 16 * (slot - 1), 128, 16, Empty);
		}
	}
}

// 删除指定 slot（1..4），执行模块删除和 flash 中记录删除（不显示提示词）
void Delete_Fingerprint(uint8_t select)
{
	if (select < 1 || select > FINGERPRINT_SLOT_COUNT)
		return;

	if (!Flash_HasSlot(select))
	{
		OLED_Clear();
		OLED_ShowString(28, 28, "无效删除！", OLED_8X16);
		OLED_Update();
		Buzzer_OLED_short();
		HAL_Delay(400);
	}
	else
	{
		// 模块内 pageID = slot - 1
		uint16_t pageID = (uint16_t)(select - 1);

		// 先在模块中删除模板（忽略返回值，由调用者决定提示）
		AS608_Delete(pageID);

		// 再在 flash 中删除记录
		FingerprintFlash_RemoveSlot(select);

		OLED_Clear();
		OLED_ShowString(28, 28, "删除成功！", OLED_8X16);
		OLED_Update();
		Buzzer_OLED_long();
		HAL_Delay(600);
	}
}

uint8_t deleteflag_f = 1, select_f = 0;

uint8_t Delete_Fingerprint_Page(void)
{
	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec())
			return 0; /* 自动回锁:超时自动关锁退出页面 */
		KeyNum = Key_GetNum();
		if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
		select_f = 0;
		if ((KeyNum >= 1 && KeyNum <= 12) | KeyNum == 15)
		{
			return 0;
		}

		switch (KeyNum)
		{
		case 13: // 上一项
			deleteflag_f--;
			if (deleteflag_f <= 0)
				deleteflag_f = 4;
			break;
		case 14: // 下一项
			deleteflag_f++;
			if (deleteflag_f >= 5)
				deleteflag_f = 1;
			break;
		case 16: // 确认
			OLED_Clear();
			OLED_Update();
			select_f = deleteflag_f;
			break;
		}
		switch (deleteflag_f)
		{
		case 1:
			Delete_Fingerprint_UI();
			OLED_ReverseArea(0, 0, 88, 16);
			OLED_Update();
			break;
		case 2:
			Delete_Fingerprint_UI();
			OLED_ReverseArea(0, 16, 88, 16);
			OLED_Update();
			break;
		case 3:
			Delete_Fingerprint_UI();
			OLED_ReverseArea(0, 32, 88, 16);
			OLED_Update();
			break;
		case 4:
			Delete_Fingerprint_UI();
			OLED_ReverseArea(0, 48, 88, 16);
			OLED_Update();
			break;
		}
		Delete_Fingerprint(select_f);
	}
}

/*-----------------------------------------开锁日志-------------------------------------------*/
/* 开锁日志页:显示最近3条,任意键返回 */
uint8_t AuditLog_Page(void)
{
	static const char *chname[4] = {"PWD", "CARD", "FING", "BT"};

	while (1)
	{
		WDT_Feed();    /* 看门狗喂狗 */
		if (AutoRelock_CheckAndExec()) return 0;   /* 自动回锁:超时自动关锁退出页面 */
		KeyNum = Key_GetNum();
		if (KeyNum) Security_RelockExtend();   /* 有按键：刷新自动回锁倒计时 */
		if (KeyNum) return 0;

		uint16_t total = AuditLog_GetCount();
		uint16_t start = (total >= 3) ? (total - 3) : 0;   /* 最近3条起始索引 */

		OLED_Clear();
		OLED_ShowString(8, 0, "开锁日志", OLED_8X16);
		for (uint8_t i = 0; i < 3 && (start + i) < total; i++)
		{
			uint16_t e = AuditLog_GetEntry(start + i);
			uint8_t ch = e & 0x0F;          /* 通道 */
			uint8_t ok = (e >> 4) & 0x01;   /* 结果(0败 1成) */
			uint8_t seq = (e >> 8) & 0xFF;  /* 序列 */
			OLED_ShowNum(0, 16 + i * 16, seq, 3, OLED_8X16);
			OLED_ShowString(32, 16 + i * 16, (ch < 4) ? (char*)chname[ch] : "?", OLED_8X16);
			OLED_ShowString(72, 16 + i * 16, ok ? "OK" : "ERR", OLED_8X16);
		}
		OLED_Update();
		HAL_Delay(150);
	}
}
