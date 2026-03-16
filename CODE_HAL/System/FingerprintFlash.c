#include "FingerprintFlash.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#define SLOT_ADDR(i) (FINGERPRINT_FLASH_PAGE_ADDR + (uint32_t)((i) * sizeof(uint16_t)))

static uint16_t FF_ReadSlot(uint8_t idx)
{
    if (idx >= FINGERPRINT_SLOT_COUNT) return FINGERPRINT_EMPTY_SLOT;
    return *(uint16_t *)(uintptr_t)SLOT_ADDR(idx);
}

static uint8_t FF_WriteAll(uint16_t *arr)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FINGERPRINT_FLASH_PAGE_ADDR;
    EraseInitStruct.NbPages = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        HAL_FLASH_Lock();
        return 0;
    }

    for (uint8_t i = 0; i < FINGERPRINT_SLOT_COUNT; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, SLOT_ADDR(i), (uint32_t)arr[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return 0;
        }
    }

    HAL_FLASH_Lock();
    return 1;
}

uint8_t FingerprintFlash_Add(uint16_t id)
{
    if (id == FINGERPRINT_EMPTY_SLOT) return 0;

    // 读取当前槽
    uint16_t arr[FINGERPRINT_SLOT_COUNT];
    uint8_t i;
    for (i = 0; i < FINGERPRINT_SLOT_COUNT; i++) arr[i] = FF_ReadSlot(i);

    // 去重
    for (i = 0; i < FINGERPRINT_SLOT_COUNT; i++) {
        if (arr[i] == id) return 1; // 已存在当作成功
    }

    // 找第一个空槽
    for (i = 0; i < FINGERPRINT_SLOT_COUNT; i++) {
        if (arr[i] == FINGERPRINT_EMPTY_SLOT) {
            // 直接写单半字需确保该地址为 0xFFFF（擦除后），此处统一采用整页擦除+写回方式以保证一致性
            arr[i] = id;
            return FF_WriteAll(arr);
        }
    }

    // 无空槽：失败（可改为覆盖策略）
    return 0;
}

uint8_t FingerprintFlash_ReadAll(uint16_t *ids, uint8_t max)
{
    if (!ids || max == 0) return 0;
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < FINGERPRINT_SLOT_COUNT && cnt < max; i++) {
        uint16_t v = FF_ReadSlot(i);
        if (v != FINGERPRINT_EMPTY_SLOT) {
            ids[cnt++] = v;
        }
    }
    return cnt;
}

uint8_t FingerprintFlash_ClearAll(void)
{
    uint16_t blank[FINGERPRINT_SLOT_COUNT];
    for (uint8_t i = 0; i < FINGERPRINT_SLOT_COUNT; i++) blank[i] = FINGERPRINT_EMPTY_SLOT;
    return FF_WriteAll(blank);
}

// 将 slot(1..4) 存入 flash（实际在底层保存 uint16_t 值）
// 若 slot 不在 1..4 返回 0，若已存在返回 1
uint8_t FingerprintFlash_AddSlot(uint8_t slot)
{
    if (slot < 1 || slot > FINGERPRINT_SLOT_COUNT) return 0;
    // 使用原有接口，存储 slot 作为 uint16_t
    return FingerprintFlash_Add((uint16_t)slot);
}

// 读取所有已存 slot（1..4），按顺序放入 slots[] 返回个数
uint8_t FingerprintFlash_ReadSlots(uint8_t *slots, uint8_t max_slots)
{
    if (!slots || max_slots == 0) return 0;
    uint16_t tmp[FINGERPRINT_SLOT_COUNT];
    uint8_t cnt = FingerprintFlash_ReadAll(tmp, FINGERPRINT_SLOT_COUNT);
    uint8_t out = 0;
    for (uint8_t i = 0; i < cnt && out < max_slots; i++)
    {
        // 保证为有效 slot 值（1..4），否则忽略
        if (tmp[i] >= 1 && tmp[i] <= FINGERPRINT_SLOT_COUNT)
        {
            slots[out++] = (uint8_t)tmp[i];
        }
    }
    return out;
}

uint8_t FingerprintFlash_RemoveSlot(uint8_t slot)
{
    if (slot < 1 || slot > FINGERPRINT_SLOT_COUNT) return 0;

    uint16_t arr[FINGERPRINT_SLOT_COUNT];
    for (uint8_t i = 0; i < FINGERPRINT_SLOT_COUNT; i++) arr[i] = FF_ReadSlot(i);

    // 查找并移除第一个匹配的 slot 值
    uint8_t found = 0;
    for (uint8_t i = 0; i < FINGERPRINT_SLOT_COUNT; i++)
    {
        if (arr[i] == (uint16_t)slot)
        {
            arr[i] = FINGERPRINT_EMPTY_SLOT;
            found = 1;
            break;
        }
    }
    if (!found) return 0;

    // 重写整页
    return FF_WriteAll(arr);
}


// 返回 flash 中已存 slot 数量
uint8_t FingerprintFlash_Count(void)
{
    uint16_t tmp[FINGERPRINT_SLOT_COUNT];
    return FingerprintFlash_ReadAll(tmp, FINGERPRINT_SLOT_COUNT);
}
