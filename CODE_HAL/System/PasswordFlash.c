#include "main.h"
#include "PasswordFlash.h"
#include <string.h>

/**
 * @brief  Flash解锁（F1系列专用）
 * @note   F1系列HAL库通过HAL_FLASH_Unlock()直接解锁，内部会判断锁定状态
 */
void Flash_Unlock(void) {
    HAL_FLASH_Unlock();  // 调用F1 HAL库解锁函数
}

/**
 * @brief  Flash锁定（F1系列专用）
 * @note   操作完成后必须锁定，防止误写入
 */
void Flash_Lock(void) {
    HAL_FLASH_Lock();  // 调用F1 HAL库锁定函数
}

/**
 * @brief  擦除密码所在的Flash页（F1系列专用）
 * @note   F1系列Flash按页擦除，每页大小根据型号不同为1KB或2KB
 *         这里针对STM32F103C8T6（每页1KB），只擦除1页
 */
void Flash_Erase(void) {
    FLASH_EraseInitTypeDef erase_init;  // 擦除配置结构体
    uint32_t error_page;                // 用于存储擦除失败时的错误页地址。如果擦除成功，该值无意义；如果失败，会被设置为擦除出错的页地址。

    // 配置擦除参数
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;  // 按页擦除
    erase_init.PageAddress = PASSWORD_FLASH_ADDR;  // 擦除起始地址（密码存储页）
    erase_init.NbPages = 1;                        // 只擦除1页
    erase_init.Banks = FLASH_BANK_1;               // F103只有Bank1

    Flash_Unlock();  // 先解锁

    // 执行擦除操作（F1的擦除函数返回状态码）
    if (HAL_FLASHEx_Erase(&erase_init, &error_page) != HAL_OK) {
        // 擦除失败可添加错误处理（如LED报警）
    }
}

/**
 * @brief  初始化密码Flash（上电时调用）
 * @note   首次上电时Flash无有效密码，自动写入默认密码"1106"
 *         后续上电时读取已有密码
 */
void PasswordFlash_Init(void) {
    PasswordStore_t init_pass;  // 用于存储默认密码的结构体
    // 读取Flash中的验证标识（将Flash地址强制转换为uint32_t指针）
    uint32_t* flash_mark = (uint32_t*)PASSWORD_FLASH_ADDR;

    // 判断Flash中是否有有效密码（验证标识是否匹配）
    if (*flash_mark != PASSWORD_VALID_MARK) {
        // 初始化默认密码数据
        init_pass.valid_mark = PASSWORD_VALID_MARK;  // 设置有效标识
        memcpy(init_pass.password, Password_Default, PASSWORD_LEN);  // 默认密码

        Flash_Erase();  // 擦除页（为写入新数据做准备）

        // 按半字（16位）写入Flash（F1系列推荐半字写入，避免对齐问题）
        uint16_t* write_data = (uint16_t*)&init_pass;  // 结构体转换为16位指针
        for (uint8_t i = 0; i < sizeof(PasswordStore_t)/2; i++) {
            //指定写入数据的粒度（大小），FLASH_TYPEPROGRAM_HALFWORD 表示 以半字（16 位，2 字节）为单位写入
            // 写入地址：起始地址 + i*2（每次偏移2字节）
            // 写入数据：结构体的第i个半字
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                             PASSWORD_FLASH_ADDR + i*2,
                             write_data[i]);
        }
    }

    Flash_Lock();  // 初始化完成后锁定Flash
}

/**
 * @brief  写入新密码到Flash（修改密码时调用）
 * @param  new_pass：新密码字符串（长度必须为PASSWORD_LEN）
 * @retval 1：写入成功；0：写入失败
 */
uint8_t PasswordFlash_Write(const char* new_pass) {
    // 校验密码长度（必须为4位）
    if (strlen(new_pass) != PASSWORD_LEN) {
        return 0;  // 长度错误，返回失败
    }

    PasswordStore_t new_data;  // 存储新密码的结构体
    new_data.valid_mark = PASSWORD_VALID_MARK;  // 设置有效标识
    memcpy(new_data.password, new_pass, PASSWORD_LEN);  // 复制新密码

    Flash_Erase();  // 擦除旧密码所在的页

    // 按半字写入新密码
    uint16_t* write_data = (uint16_t*)&new_data;
    for (uint8_t i = 0; i < sizeof(PasswordStore_t)/2; i++) {
        // 调用F1的写入函数，若失败则锁定并返回0
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                             PASSWORD_FLASH_ADDR + i*2,
                             write_data[i]) != HAL_OK) {
            Flash_Lock();
            return 0;  // 写入失败
        }
    }

    Flash_Lock();  // 写入成功后锁定
    return 1;      // 写入成功
}

/**
 * @brief  从Flash读取密码（验证密码时调用）
 * @param  buff：存储读取结果的缓冲区（需至少PASSWORD_LEN+1字节）
 * @retval 1：读取成功；0：读取失败（数据无效）
 */
uint8_t PasswordFlash_Read(char* buff) {
    // 将Flash地址强制转换为密码结构体指针（F1的Flash地址可直接访问）
    PasswordStore_t* stored_pass = (PasswordStore_t*)PASSWORD_FLASH_ADDR;

    // 校验有效标识（确保数据未损坏）
    if (stored_pass->valid_mark != PASSWORD_VALID_MARK) {
        return 0;  // 数据无效，返回失败
    }

    // 复制密码到缓冲区，并补充字符串结束符
    memcpy(buff, stored_pass->password, PASSWORD_LEN);
    buff[PASSWORD_LEN] = '\0';  // 确保字符串正确结束

    return 1;  // 读取成功
}
