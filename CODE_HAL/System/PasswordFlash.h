#ifndef __PASSWORDFLASH_H__
#define __PASSWORDFLASH_H__

#include "main.h"

// 密码长度（与现有代码保持一致）
#define PASSWORD_LEN 4
// Flash存储地址（page51）
#define PASSWORD_FLASH_ADDR 0x0800CC00  // 地址范围：0x0800CC00 ~ 0x0800CFFF
// 验证标识（用于判断Flash数据是否有效）
#define PASSWORD_VALID_MARK_XOR 0x5A5A5A5B  /* 有效标识:密码已异或加扰 */
#define PASSWORD_XOR_KEY        0x5A        /* 异或加扰密钥 */
//默认密码
#define Password_Default "1106"

// 密码存储结构体（大小需为2字节的倍数，适配F1的半字写入）
typedef struct {
    uint32_t valid_mark;       // 4字节验证标识
    char password[PASSWORD_LEN];// 4字节密码
} PasswordStore_t;  // 总大小8字节，满足2字节对齐

// 函数声明
void PasswordFlash_Init(void);                  // 初始化（上电读取或写入默认密码）
uint8_t PasswordFlash_Write(const char* new_pass); // 写入新密码到Flash
uint8_t PasswordFlash_Read(char* buff);         // 从Flash读取密码

#endif

