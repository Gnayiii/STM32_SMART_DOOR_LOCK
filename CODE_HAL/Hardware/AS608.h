#ifndef __AS608_H
#define __AS608_H

#include "main.h"  // 包含 HAL 头文件、huart2 等
#include <stdint.h>
#include <string.h>

// 返回确认码定义（与 EnsureMessage 一致）
#define AS608_OK                        0x00
#define AS608_PACKET_RX_ERROR           0x01
#define AS608_NO_FINGER                 0x02
#define AS608_ENROLL_FAIL               0x03
#define AS608_IMG_TOO_DRY               0x04
#define AS608_IMG_TOO_WET               0x05
#define AS608_IMG_TOO_MESSY             0x06
#define AS608_FEW_FEATURE_POINTS        0x07
#define AS608_NOT_MATCH                 0x08
#define AS608_NOT_FOUND                 0x09  // 实际模块常返回 0x09 表“未匹配”
#define AS608_MERGE_FAIL                0x0A
#define AS608_ADDR_OUT_OF_RANGE         0x0B
#define AS608_DEL_FAIL                  0x10
#define AS608_EMPTY_FAIL                0x11
#define AS608_NO_IMAGE                  0x15
#define AS608_FLASH_ERROR               0x18
#define AS608_INVALID_REG_NUM           0x1A
#define AS608_INVALID_REG_VAL           0x1B
#define AS608_INVALID_NOTE_PAGE         0x1C
#define AS608_DB_FULL                   0x1F
#define AS608_ADDR_ERROR                0x20

// 搜索结果结构体
typedef struct {
    uint16_t pageID;      // 页码（模板ID）
    uint16_t mathscore;   // 匹配分数（0~10000，越大越相似）
} SearchResult;

// 系统参数结构体
typedef struct {
    uint16_t PS_max;      // 最大指纹容量
    uint8_t  PS_level;    // 对比等级（1~5）
    uint32_t PS_addr;     // 模块地址
    uint8_t  PS_size;     // 包大小（0=32B, 1=64B, 2=128B, 3=256B）
    uint8_t  PS_N;        // 波特率系数：实际波特率 = PS_N * 9600
} SysPara;

// AS608 全局地址（默认广播地址）
extern uint32_t AS608Addr;

// 函数声明
void AS608_StartReceiveIT(void);
void AS608_RxPush(uint8_t byte);
uint8_t PS_HandShake(uint32_t *pAddr);
uint8_t PS_GetImage(void);
uint8_t PS_GenChar(uint8_t BufferID);
uint8_t PS_Match(void);
uint8_t PS_Search(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p);
uint8_t PS_RegModel(void);
uint8_t PS_StoreChar(uint8_t BufferID, uint16_t PageID);
uint8_t PS_DeletChar(uint16_t PageID, uint16_t N);
uint8_t PS_Empty(void);
uint8_t PS_WriteReg(uint8_t RegNum, uint8_t DATA);
uint8_t PS_ReadSysPara(SysPara *p);
uint8_t PS_SetAddr(uint32_t PS_addr);
uint8_t PS_WriteNotepad(uint8_t NotePageNum, uint8_t *Byte32);
uint8_t PS_ReadNotepad(uint8_t NotePageNum, uint8_t *Byte32);
uint8_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p);
uint8_t PS_ValidTempleteNum(uint16_t *ValidN);
uint8_t PS_ReadTouch(void);  // 读取 PA0 触摸状态：1=有手指，0=无
uint8_t AS608_IdentifyOnce(uint16_t *matched_id, uint32_t timeout_ms);// 1 = 匹配成功，0 = 未匹配或超时或出错
uint8_t AS608_Add(uint16_t pageID, uint32_t timeout_ms);
uint8_t AS608_Delete(uint16_t pageID);
uint8_t AS608_ClearAll(void); // 清空模块内所有模板

const char* EnsureMessage(uint8_t ensure);

// 内部用：接收数据包（带超时）
uint8_t* AS608_RecvPacket(uint32_t timeout_ms);

#endif /* __AS608_H */
