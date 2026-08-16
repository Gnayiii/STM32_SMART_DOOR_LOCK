#ifndef __SECURITY_H__
#define __SECURITY_H__
#include <stdint.h>

/* 认证通道编号:键盘 / RFID / 指纹 / 蓝牙 */
#define CH_KEYPAD 0
#define CH_RFID 1
#define CH_FP 2
#define CH_BT 3
#define AUTH_MAX_FAIL 5   /* 连续失败阈值 */
#define LOCKOUT_MS 30000u /* 暴力破解锁定时长30s */
#define RELOCK_MS 30000u  /* 自动回锁延时30s */

/* ---- 开锁审计日志(Flash 页57 = 0x0800E400) ---- */
#define AUDIT_FLASH_ADDR 0x0800E400u /* 日志页地址 */
#define AUDIT_MAGIC 0xA7A7A7A7u      /* 有效标志(magic 最后写,作提交点) */
#define AUDIT_CAPACITY 16            /* 日志容量(条) */

typedef struct
{
    uint32_t magic;                   /* 有效标志 */
    uint16_t count;                   /* 已记录条数 */
    uint16_t next_seq;                /* 下一条序列号 */
    uint16_t entries[AUDIT_CAPACITY]; /* entry:bit0-3通道,bit4结果,bit8-15序列 */
} AuditStore_t;                       /* 40字节 */

void Security_Init(void);                     /* 初始化(清零所有状态) */
void Security_OnAuthFail(uint8_t channel);    /* 认证失败打点(达到阈值触发锁定) */
void Security_OnAuthSuccess(uint8_t channel); /* 认证成功打点(清零计数+启动自动回锁) */
uint8_t Security_IsLocked(void);              /* 是否锁定?超时自动解锁 */
uint32_t Security_LockRemainMs(void);         /* 剩余锁定毫秒数(未锁定返回0) */
uint8_t Security_RelockDue(void);             /* 自动回锁到期?到期清标志返回1 */
void Security_ClearRelock(void);              /* 取消自动回锁(手动关锁时) */
void Security_RelockExtend(void);             /* 活动后续期：重置自动回锁倒计时 */

/* 开锁审计日志(Flash 页57 = 0x0800E400) */
void AuditLog_Init(void);                              /* 初始化(读侧以 magic 判空,无需预擦除) */
void AuditLog_Append(uint8_t channel, uint8_t result); /* 追加日志(0失败 1成功) */
uint16_t AuditLog_GetCount(void);                      /* 当前日志条数(0~16) */
uint16_t AuditLog_GetEntry(uint8_t index);             /* 读取第 index 条(0=最早) */
#endif
