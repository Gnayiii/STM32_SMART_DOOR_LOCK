#include "stm32f1xx_hal.h"  // 包含 HAL 库，自动包含 stdint.h 等

/**
  * @brief   flash写入数据 
  * @param   add 32位flash地址
  * @param   dat1~dat4: 每个为8位数据（按原逻辑，各自作为16位值写入，高8位为0）
  * @retval  无
  */
void FLASH_W(uint32_t add, uint8_t dat1, uint8_t dat2, uint8_t dat3, uint8_t dat4)
{
    HAL_FLASH_Unlock();

    // 擦除 add 所在的页（STM32F1 页大小为 1KB = 0x400）
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = add & 0xFFFFFC00U; // 对齐到 1KB 边界
    EraseInitStruct.NbPages = 1;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);

    // 按原函数逻辑：每个 uint8_t 作为 16 位写入（低8位=数据，高8位=0）
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, add,     (uint16_t)dat1);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, add + 2, (uint16_t)dat2);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, add + 4, (uint16_t)dat3);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, add + 6, (uint16_t)dat4);

    HAL_FLASH_Lock();
}

/**
  * @brief    FLASH读出数据
  * @param    add 32位读出FLASH地址
  * @retval   16位数据
  */
uint16_t FLASH_R(uint32_t add)
{
    return *(uint16_t*)add;
}

/**
  * @brief    擦除指定FLASH地址页内的内容
  * @param    add 32位FLASH地址
  * @retval   无
  */
void FLASH_Clear(uint32_t add)
{
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = add & 0xFFFFFC00U;
    EraseInitStruct.NbPages = 1;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);

    HAL_FLASH_Lock();
}

