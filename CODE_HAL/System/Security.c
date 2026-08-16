#include "Security.h"
#include "stm32f1xx_hal.h"
#include <string.h>

/* ---- 反暴力锁定状态 ---- */
static uint8_t fail_count = 0;       /* 连续认证失败次数 */
static uint8_t locked = 0;           /* 锁定标志 */
static uint32_t lock_start_tick = 0; /* 锁定开始时刻 */
/* ---- 自动回锁状态 ---- */
static uint8_t relock_enabled = 0;   /* 自动回锁使能 */
static uint32_t relock_deadline = 0; /* 自动回锁截止时刻 */

/* 初始化:清零所有状态 */
void Security_Init(void)
{
    fail_count = 0;
    locked = 0;
    relock_enabled = 0;
    AuditLog_Init(); /* 审计日志初始化(读侧以 magic 判空) */
}

/* 认证失败:计数+1,达到阈值则锁定 */
void Security_OnAuthFail(uint8_t channel)
{
    if (locked)
        return; /* 已锁定则忽略 */
    fail_count++;
    AuditLog_Append(channel, 0); /* 记录失败日志 */
    if (fail_count >= AUTH_MAX_FAIL)
    {
        locked = 1;
        lock_start_tick = HAL_GetTick();
    }
}

/* 认证成功:清零失败计数并解锁,同时启动自动回锁定时 */
void Security_OnAuthSuccess(uint8_t channel)
{
    fail_count = 0;
    locked = 0;
    AuditLog_Append(channel, 1); /* 记录成功日志 */
    relock_enabled = 1;
    relock_deadline = HAL_GetTick() + RELOCK_MS;
}

/* 锁定态查询;超时自动解锁并清零计数 */
uint8_t Security_IsLocked(void)
{
    if (locked && (HAL_GetTick() - lock_start_tick) >= LOCKOUT_MS)
    {
        locked = 0;
        fail_count = 0;
    }
    return locked;
}

/* 剩余锁定毫秒数(未锁定返回0) */
uint32_t Security_LockRemainMs(void)
{
    if (!locked)
        return 0;
    uint32_t el = HAL_GetTick() - lock_start_tick;
    return (el >= LOCKOUT_MS) ? 0 : (LOCKOUT_MS - el);
}

/* 自动回锁到期判断;有符号差防 tick 回绕 */
uint8_t Security_RelockDue(void)
{
    if (!relock_enabled)
        return 0;
    if ((int32_t)(HAL_GetTick() - relock_deadline) >= 0)
    {
        relock_enabled = 0;
        return 1;
    }
    return 0;
}

/* 取消自动回锁(手动关锁时调用) */
void Security_ClearRelock(void) { relock_enabled = 0; }

/* 活动后续期：解锁态用户每次操作，把回锁截止时间刷新为“现在+RELOCK_MS” */
void Security_RelockExtend(void)
{
    if (relock_enabled)                        /* 仅当自动回锁已开启（解锁态）才续期 */
        relock_deadline = HAL_GetTick() + RELOCK_MS;
}

/* 开锁审计日志初始化 */
void AuditLog_Init(void)
{
    /* 读侧以 magic 判空,无需预擦除 */
}

/* 追加一条日志:整页擦除后按序写,magic 最后写(掉电半写则下次按空处理) */
void AuditLog_Append(uint8_t channel, uint8_t result)
{
    AuditStore_t s;
    if (*(volatile uint32_t *)AUDIT_FLASH_ADDR == AUDIT_MAGIC)
        memcpy(&s, (const void *)AUDIT_FLASH_ADDR, sizeof(s)); /* 读当前日志 */
    else
    {
        memset(&s, 0, sizeof(s));
        s.count = 0;
        s.next_seq = 0;
    } /* 无效则从空开始 */

    if (s.count < AUDIT_CAPACITY)
    { /* 未满:末尾追加 */
        s.entries[s.count] = (uint16_t)(((s.next_seq & 0xFFu) << 8) | (channel & 0x0Fu) | (result ? 0x10u : 0u));
        s.count++;
    }
    else
    { /* 已满:丢最旧,整体前移 */
        for (uint8_t i = 0; i < AUDIT_CAPACITY - 1; i++)
            s.entries[i] = s.entries[i + 1];
        s.entries[AUDIT_CAPACITY - 1] = (uint16_t)(((s.next_seq & 0xFFu) << 8) | (channel & 0x0Fu) | (result ? 0x10u : 0u));
    }
    s.next_seq++;

    FLASH_EraseInitTypeDef ei;
    uint32_t err = 0;
    HAL_FLASH_Unlock();
    ei.TypeErase = FLASH_TYPEERASE_PAGES;
    ei.PageAddress = AUDIT_FLASH_ADDR;
    ei.NbPages = 1;
    ei.Banks = FLASH_BANK_1; /* 与 Flash.c 风格一致(页擦除不依赖此字段) */
    HAL_FLASHEx_Erase(&ei, &err);
    uint16_t *w = (uint16_t *)&s;
    for (uint8_t i = 2; i < sizeof(s) / 2; i++) /* 先写 magic 之后的所有字段 */
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, AUDIT_FLASH_ADDR + (uint32_t)i * 2, w[i]);
    for (uint8_t i = 0; i < 2; i++) /* magic 最后写 = 提交点，避免数据写入途中掉电，导致数据丢失 */
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, AUDIT_FLASH_ADDR + (uint32_t)i * 2, w[i]);
    HAL_FLASH_Lock();
}

/* 当前日志条数(magic 无效返回0) */
uint16_t AuditLog_GetCount(void)
{
    return (*(volatile uint32_t *)AUDIT_FLASH_ADDR == AUDIT_MAGIC) ? *(volatile uint16_t *)(AUDIT_FLASH_ADDR + 4) : 0;
}

/* 读取第 index 条(0=最早);越界返回0 */
uint16_t AuditLog_GetEntry(uint8_t index)
{
    if (index >= AUDIT_CAPACITY)
        return 0;
    return *(volatile uint16_t *)(AUDIT_FLASH_ADDR + 8 + (uint32_t)index * 2);
}
