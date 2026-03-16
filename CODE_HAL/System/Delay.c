#include "Delay.h"
#include "main.h"

void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < cycles);
}

void delay_ms(uint32_t ms)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = ms * (SystemCoreClock / 1000U);
    while ((DWT->CYCCNT - start) < cycles);
}