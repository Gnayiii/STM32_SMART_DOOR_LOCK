/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "Key.h"
#include "Motor.h"
#include "Menu.h"
#include "PasswordFlash.h"
#include "MFRC522.h"
#include "Delay.h"
#include "Flash.h"
#include "BUZZER.h"
#include "AS608.h"
#include "Security.h"
#include "WDT.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t cardnumber;
uint8_t RxData[PASSWORD_LEN + 1];
volatile uint8_t bt_rx_buf[PASSWORD_LEN]; /* 蓝牙帧缓冲，最多4位数字 */
volatile uint8_t bt_rx_len;               /* 已收到数字个数 */
volatile uint32_t bt_last_tick;           /* 最近收到字节的时刻（超时判半帧） */
uint8_t bt_rx_byte;                       /* 蓝牙单字节接收单元 */
static uint8_t rfid_fail_pending = 0;     /* RFID 防重复计数闩锁 */
extern uint8_t LockFlag;
extern uint8_t Flag;
extern uint8_t deleteflag;
extern uint8_t deleteflag_f;
extern uint8_t usart2_rx_byte; // 用于 HAL_UART_Receive_IT 接收

extern uint8_t UID[4], Temp[4];
extern uint8_t UI0[4]; // 卡片0ID数组
extern uint8_t UI1[4]; // 卡片1ID数组
extern uint8_t UI2[4]; // 卡片2ID数组
extern uint8_t UI3[4]; // 卡片3ID数组

void RFID_Check(void); // 声明函数
void Read_Card(void);
uint8_t Check_Password_HC06(void);

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init(); // 初始化Delay
  OLED_Init();
  RFID_Init();
  PasswordFlash_Init();
  Security_Init();                                  /* 反暴力+自动回锁初始化 */
  WDT_Init();                                       /* 独立看门狗初始化 */
  HAL_UART_Receive_IT(&huart2, &usart2_rx_byte, 1); // 启动 USART2 单字节接收（保持原样）
  HAL_UART_Receive_IT(&huart1, &bt_rx_byte, 1);     /* 蓝牙：单字节持续接收，杜绝寄存器残留 */
  OLED_Clear();
  HAL_TIM_Base_Start_IT(&htim2);
  Menu_Show_UI(); // 入场动画

  uint8_t FirstFlag = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    WDT_Feed();        /* 看门狗喂狗 */
    Read_Card();       // 读取RFID卡号
    if (LockFlag == 0) // 门锁关闭状态
    {
      /* 蓝牙：凑满4位数字即一帧，半帧300ms无新字节作废 */
      if (bt_rx_len == PASSWORD_LEN)
      {
        for (uint8_t i = 0; i < PASSWORD_LEN; i++)
          RxData[i] = bt_rx_buf[i];
        RxData[PASSWORD_LEN] = '\0';
        bt_rx_len = 0;
        if (!Security_IsLocked() && Check_Password_HC06()) /* 反暴力:锁定期间忽略蓝牙 */
        {
          OLED_Clear();
          OLED_ShowString(28, 28, "解锁成功！", OLED_8X16);
          OLED_Update();
          Buzzer_Lock();
          Motor_DirectionAngle90(ccw);
          Flag = 1;
          LockFlag = 1;
          Security_OnAuthSuccess(CH_BT); /* 反暴力:成功清零+启动回锁 */
        }
        else if (!Security_IsLocked()) /* 反暴力:锁定期间忽略蓝牙 */
        {
          Security_OnAuthFail(CH_BT); /* 反暴力:失败计数 */
          OLED_Clear();
          OLED_ShowString(28, 28, "密码错误！", OLED_8X16);
          OLED_Update();
          Buzzer_OLED_long();
          HAL_Delay(600);
          OLED_Clear();
        }
      }
      else if (bt_rx_len && (HAL_GetTick() - bt_last_tick) > 300)
        bt_rx_len = 0; /* 半帧超时作废，防残留拼接 */
      // 指纹解锁
      if (!Security_IsLocked() && PS_ReadTouch() == 1) // 有手指触摸（反暴力:锁定期间忽略指纹）
      {
        uint16_t fingerID;
        if (AS608_IdentifyOnce(&fingerID, 3000) == 0) // 指纹识别
        {
          Security_OnAuthFail(CH_FP); /* 反暴力:失败计数 */
          OLED_Clear();
          OLED_ShowString(20, 28, "指纹未匹配!", OLED_8X16);
          OLED_Update();
          Buzzer_OLED_long();
          uint32_t t = HAL_GetTick(); /* 看门狗:死等改有界+喂狗 */
          while (PS_ReadTouch() && (HAL_GetTick() - t) < 5000)
          {
            WDT_Feed();
            HAL_Delay(50);
          }
        }
        else
        {
          OLED_Clear();
          OLED_ShowString(32, 28, "指纹ID:", OLED_8X16);
          OLED_ShowNum(88, 28, fingerID + 1, 1, OLED_8X16);
          OLED_Update();
          Buzzer_Lock();
          Motor_DirectionAngle90(ccw);
          Flag = 1;
          LockFlag = 1;
          Security_OnAuthSuccess(CH_FP); /* 反暴力:成功清零+启动回锁 */
        }
      }
      RFID_Check();
      Lock_Page();
    }
    else // 门锁开启状态
    {
      bt_rx_len = 0; /* 开锁态丢弃蓝牙残帧，防回锁后误触发 */
      if (AutoRelock_CheckAndExec())
        continue; /* 自动回锁:超时自动关锁回锁屏 */
      // S13=上一页，返回
      // S14=上一页
      // S15=删除
      // S16=确认
      OLED_Clear();
      Menu_UI();
      OLED_Update();

      FirstFlag = First_Page();
      if (FirstFlag == 1) // 修改密码
      {
        OldPassword_Page();
      }
      else if (FirstFlag == 2) // 录入新卡
      {
        Add_Card_Page();
      }
      else if (FirstFlag == 3) // 删除卡片
      {
        deleteflag = 1;
        Delete_Card_Page();
      }
      else if (FirstFlag == 4) // 关锁
      {
        Security_ClearRelock(); /* 手动关锁:取消自动回锁 */
        OLED_Clear();
        OLED_ShowString(20, 28, "正在关锁", OLED_8X16);
        OLED_ShowChar(84, 28, '.', OLED_8X16);
        OLED_ShowChar(92, 28, '.', OLED_8X16);
        OLED_ShowChar(100, 28, '.', OLED_8X16);
        OLED_Update();
        Motor_DirectionAngle90(cw);
        bt_rx_len = 0;          /* 丢弃菜单页期间蓝牙残留,防关锁后误触发 */
        LockFlag = 0;
      }
      else if (FirstFlag == 5) // 录入指纹
      {
        Add_Fingerprint_Page();
      }
      else if (FirstFlag == 6) // 删除指纹
      {
        deleteflag_f = 1;
        Delete_Fingerprint_Page();
      }
      else if (FirstFlag == 7) // 查看开锁日志
      {
        AuditLog_Page();
      }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

uint8_t Check_Password_HC06(void)
{
  char stored_pass[PASSWORD_LEN + 1];
  // 从Flash中读取密码，如果读取失败，则默认使用默认密码"1106"
  if (PasswordFlash_Read(stored_pass) != 1)
  {
    return strcmp((char *)RxData, Password_Default) == 0;
  }
  // 比较接收到的密码是否正确
  return strcmp((char *)RxData, stored_pass) == 0;
}

// RFID卡检测函数（反暴力:锁定期间忽略+防重复计数）
void RFID_Check(void)
{
  if (Security_IsLocked())
  {
    rfid_fail_pending = 0;
    return;
  } // 锁定期间忽略并清闩锁
  cardnumber = Rc522Test(); // 读取卡片ID
  if (cardnumber == 0)      // 有卡但未录入
  {
    if (!rfid_fail_pending)
    {
      rfid_fail_pending = 1;
      Security_OnAuthFail(CH_RFID);
    } // 卡压住不拿走只计一次失败
    OLED_Clear();
    OLED_ShowString(28, 28, "卡片错误！", OLED_8X16);
    OLED_Update();
    Buzzer_OLED_long();
    WaitCardOff_Timeout(5000); // 等待卡片移开
  }
  else if (cardnumber >= 1 && cardnumber <= 4) // 合法卡1-4
  {
    rfid_fail_pending = 0;
    Security_OnAuthSuccess(CH_RFID); /* 反暴力:成功清零+启动回锁 */
    OLED_Clear();
    OLED_ShowString(40, 28, "卡ID:", OLED_8X16);
    OLED_ShowNum(80, 28, cardnumber, 1, OLED_8X16);
    OLED_Update();
    LockFlag = 1;
    Flag = 1;
    Buzzer_Lock();
    Motor_DirectionAngle90(ccw);
    WaitCardOff_Timeout(5000); // 等待卡片移开
  }
  else if (cardnumber == 6)
  {
    rfid_fail_pending = 0;
  } // 无卡:清闩锁
}

// 从flash中读取各卡信息
void Read_Card()
{
  UI0[0] = FLASH_R(FLASH_ADDR1);
  UI0[1] = FLASH_R(FLASH_ADDR1 + 2);
  UI0[2] = FLASH_R(FLASH_ADDR1 + 4);
  UI0[3] = FLASH_R(FLASH_ADDR1 + 6);

  UI1[0] = FLASH_R(FLASH_ADDR2);
  UI1[1] = FLASH_R(FLASH_ADDR2 + 2);
  UI1[2] = FLASH_R(FLASH_ADDR2 + 4);
  UI1[3] = FLASH_R(FLASH_ADDR2 + 6);

  UI2[0] = FLASH_R(FLASH_ADDR3);
  UI2[1] = FLASH_R(FLASH_ADDR3 + 2);
  UI2[2] = FLASH_R(FLASH_ADDR3 + 4);
  UI2[3] = FLASH_R(FLASH_ADDR3 + 6);

  UI3[0] = FLASH_R(FLASH_ADDR4);
  UI3[1] = FLASH_R(FLASH_ADDR4 + 2);
  UI3[2] = FLASH_R(FLASH_ADDR4 + 4);
  UI3[3] = FLASH_R(FLASH_ADDR4 + 6);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // TIM2中断回调函数
  if (htim->Instance == TIM2)
  {
    Key_Tick(); // 每20ms执行一次按键扫描
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    if (bt_rx_len && bt_rx_len < PASSWORD_LEN && (HAL_GetTick() - bt_last_tick) > 300)
      bt_rx_len = 0; /* 阻塞期间半帧也按300ms作废，防残留续帧 */
    bt_last_tick = HAL_GetTick();
    if (bt_rx_len < PASSWORD_LEN && bt_rx_byte >= '0' && bt_rx_byte <= '9')
      bt_rx_buf[bt_rx_len++] = bt_rx_byte;        /* 只收数字，最多4位，其余丢弃 */
    HAL_UART_Receive_IT(&huart1, &bt_rx_byte, 1); /* 持续武装1字节，字节到即消费，无残留 */
  }
  if (huart->Instance == USART2)
  {
    AS608_RxPush(usart2_rx_byte);                     // 把字节推给 AS608 环形缓冲
    HAL_UART_Receive_IT(&huart2, &usart2_rx_byte, 1); // 重新启动下一次接收
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
