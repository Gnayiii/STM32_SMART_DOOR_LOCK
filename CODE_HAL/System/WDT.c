#include "WDT.h"

static IWDG_HandleTypeDef hiwdg;

/* 独立看门狗初始化:Prescaler=256, Reload=4095 */
/* T = (Reload + 1) × Prescaler / f_LSI */
/* LSI是粗糙的RC振荡器，频率会漂移 */
/* LSI 40kHz -> 约26.2s;LSI 60kHz(最坏)-> 约17.5s */
void WDT_Init(void)
{
    __HAL_DBGMCU_FREEZE_IWDG();  /* 调试冻结，接入调试器后看门狗随内核一起暂停，避免断点调试时看门狗复位 */
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Reload = 4095;
    HAL_IWDG_Init(&hiwdg);       /* F1 HAL 自动启动 LSI */
}

/* 喂狗:重装计数,开始新的倒计时 */
void WDT_Feed(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}
