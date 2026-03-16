#include "AS608.h"
#include "FingerprintFlash.h"
#include "usart.h"  // 确保包含 huart2 定义

// 全局变量
uint32_t AS608Addr = 0xFFFFFFFF; // 默认广播地址

// 接收缓冲区（建议大小 >= 128）
#define RX_BUF_SIZE 256
static uint8_t rx_buffer[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static volatile uint8_t rx_overflow = 0;

/**
 * 等待手指并识别一次指纹
 * matched_id 为匹配成功的指纹ID号，不读取ID可填 NULL
 * timeout_ms 为总超时（毫秒）
 * 返回：1 = 匹配成功，0 = 未匹配或超时或出错
 */
uint8_t AS608_IdentifyOnce(uint16_t *matched_id, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    SearchResult res;

    // 等待手指放上（直到 timeout）
    while ((HAL_GetTick() - t0) < timeout_ms)
    {
        if (PS_ReadTouch()) break;
        HAL_Delay(30);
    }
    if ((HAL_GetTick() - t0) >= timeout_ms)
    {
        return 0; // 超时无手指
    }

    // 有手指，尝试读取图像（给出一个短超时用于读取图像）
    uint32_t t1 = HAL_GetTick();
    while ((HAL_GetTick() - t1) < 3000)
    {
        if (PS_GetImage() == AS608_OK) break;
        HAL_Delay(80);
    }
    if ((HAL_GetTick() - t1) >= 3000)
    {
        return 0; // 读取图像超时/失败
    }

    // 生成特征
    if (PS_GenChar(1) != AS608_OK)
    {
        return 0;
    }

    // 搜索（从 0 开始，搜索 0~127 共128个模板；如需更大范围可调整）
    if (PS_Search(1, 0, 128, &res) == AS608_OK)
    {
        if (matched_id) *matched_id = res.pageID;
        return 1; // 匹配成功
    }

    return 0; // 未匹配
}

/**
 * 录入新指纹（两次采集 + 合并 + 存储）
 * 返回：1 成功，0 失败
 */
uint8_t AS608_Add(uint16_t pageID, uint32_t timeout_ms)
{
    uint32_t t0;
    uint8_t ret;

    // 第一次按压：等待手指放上并读取图像
    t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < timeout_ms)
    {
        if (PS_ReadTouch()) break;
        HAL_Delay(20);
    }
    if ((HAL_GetTick() - t0) >= timeout_ms) return 0;

    // 读取图像
    t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < 3000)
    {
        if (PS_GetImage() == AS608_OK) break;
        HAL_Delay(80);
    }
    if ((HAL_GetTick() - t0) >= 3000) return 0;

    // 生成特征到缓冲区1
    ret = PS_GenChar(1);
    if (ret != AS608_OK) return 0;

    // 等待手指抬起
    t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < timeout_ms)
    {
        if (!PS_ReadTouch()) break;
        HAL_Delay(20);
    }
    if ((HAL_GetTick() - t0) >= timeout_ms) return 0;

    HAL_Delay(200); // 给用户时间再放第二次

    // 第二次按压：等待手指放上并读取图像
    t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < timeout_ms)
    {
        if (PS_ReadTouch()) break;
        HAL_Delay(20);
    }
    if ((HAL_GetTick() - t0) >= timeout_ms) return 0;

    // 读取图像（第2次）
    t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < 3000)
    {
        if (PS_GetImage() == AS608_OK) break;
        HAL_Delay(80);
    }
    if ((HAL_GetTick() - t0) >= 3000) return 0;

    // 生成特征到缓冲区2
    ret = PS_GenChar(2);
    if (ret != AS608_OK) return 0;

    // 合并两个特征
    ret = PS_RegModel();
    if (ret != AS608_OK) return 0;

    // 存储到 pageID
    ret = PS_StoreChar(1, pageID);
    if (ret != AS608_OK) return 0;

    return 1; // 成功
}

/**
 * 删除指定模板页（单个）
 * 返回：1 成功，0 失败
 */
uint8_t AS608_Delete(uint16_t pageID)
{
    uint8_t ret;
    // 删除 1 个模板（从 pageID 开始）
    ret = PS_DeletChar(pageID, 1);
    if (ret == AS608_OK) return 1;
    return 0;
}

// 导出：启动下一次中断接收（1字节模式）
// 注意：若项目中在别处使用另一个单字节变量接收（如 usart2_rx_byte），请只调用一次启动函数，避免冲突。
void AS608_StartReceiveIT(void) {
    static uint8_t as608_rx_tmp; // 本文件的单字节接收缓冲（安全）
    HAL_UART_Receive_IT(&huart2, &as608_rx_tmp, 1);
}

/* 导出：中断回调把一个字节推入环形缓冲 */
void AS608_RxPush(uint8_t byte)
{
    uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
    if (next != rx_tail) { // 未满
        rx_buffer[rx_head] = byte; // 必须先写入数据
        rx_head = next;
    } else {
        rx_overflow = 1; // 溢出标志
    }
}

uint8_t AS608_ClearAll(void)
{
    uint8_t ret = PS_Empty();
    if (ret == AS608_OK) return 1;
    return 0;
}

// UART2 中断回调 —— 必须放在 usart.c 或此处 extern
// 【注意】：若放在 as608.c，请确保在 usart.c 的 HAL_UART_RxCpltCallback 中调用本逻辑
// 这里我们假设你已在 usart.c 中添加了对本缓冲区的支持
// 若未实现，请将以下函数移到 usart.c，并取消注释 __weak 版本
/*
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
        if (next != rx_tail) { // 非满
            rx_head = next;
            AS608_StartReceiveIT();
        } else {
            rx_overflow = 1;
            // 可选：重启接收
            AS608_StartReceiveIT();
        }
    }
}
*/

// 从环形缓冲区拷贝 len 字节到 dst（线程安全）
static uint16_t AS608_CopyFromRing(uint8_t *dst, uint16_t len) {
    __disable_irq();
    uint16_t available = (rx_head >= rx_tail) ? (rx_head - rx_tail) : (RX_BUF_SIZE - rx_tail + rx_head);
    uint16_t copy_len = (len < available) ? len : available;
    uint16_t idx = rx_tail;
    for (uint16_t i = 0; i < copy_len; i++) {
        dst[i] = rx_buffer[idx];
        idx = (idx + 1) % RX_BUF_SIZE;
    }
    __enable_irq();
    return copy_len;
}

// 获取环形缓冲区中可读字节数
static uint16_t AS608_Available(void) {
    __disable_irq();
    uint16_t avail = (rx_head >= rx_tail) ? (rx_head - rx_tail) : (RX_BUF_SIZE - rx_tail + rx_head);
    __enable_irq();
    return avail;
}

// 清空接收缓冲区（谨慎使用）
static void AS608_FlushRx(void) {
    __disable_irq();
    rx_head = rx_tail = 0;
    rx_overflow = 0;
    __enable_irq();
}

// 【关键】等待并解析应答包
// 返回：指向包起始的静态缓冲区指针（有效期到下次调用）；NULL=超时/错误
uint8_t* AS608_RecvPacket(uint32_t timeout_ms) {
    uint32_t start = HAL_GetTick();
    static uint8_t packet_buf[128]; // 静态缓冲区，避免中断中 malloc

    while ((HAL_GetTick() - start) < timeout_ms) {
        uint16_t avail = AS608_Available();
        if (avail < 12) { // 最小应答包 12 字节
            HAL_Delay(2);
            continue;
        }

        // 拷贝当前所有数据到临时缓冲区
        uint8_t temp_buf[RX_BUF_SIZE];
        uint16_t len = AS608_CopyFromRing(temp_buf, sizeof(temp_buf));

        // 在 temp_buf 中搜索包头：0xEF 0x01 [addr4] 0x07（应答包）或 0x02（数据包）
        for (uint16_t i = 0; i <= len - 9; i++) {
            if (temp_buf[i] == 0xEF && temp_buf[i + 1] == 0x01) {
                // 检查第 6 字节：0x07=命令应答，0x02=上传数据包（如图像）
                if (temp_buf[i + 6] == 0x07 || temp_buf[i + 6] == 0x02) {
                    // 计算包长度 = (temp_buf[i+7] << 8) + temp_buf[i+8]
                    uint16_t pkg_len = (temp_buf[i + 7] << 8) | temp_buf[i + 8];
                    uint16_t total_len = 9 + pkg_len; // 包头9字节 + 数据 + 校验2字节
                    if (i + total_len <= len) {
                        // 找到完整包，拷贝到静态区
                        memcpy(packet_buf, &temp_buf[i], total_len);
                        // 移动环形缓冲区指针（消费掉前面无效数据 + 当前包）
                        __disable_irq();
                        rx_tail = (rx_tail + i + total_len) % RX_BUF_SIZE;
                        __enable_irq();
                        return packet_buf;
                    }
                }
            }
        }
        HAL_Delay(2);
    }
    return NULL;
}

// 串口发送单字节
static void MYUSART_SendData(uint8_t data) {
    HAL_UART_Transmit(&huart2, &data, 1, HAL_MAX_DELAY);
}

// 发送包头
static void SendHead(void) {
    MYUSART_SendData(0xEF);
    MYUSART_SendData(0x01);
}

// 发送地址（4字节）
static void SendAddr(void) {
    MYUSART_SendData((uint8_t)(AS608Addr >> 24));
    MYUSART_SendData((uint8_t)(AS608Addr >> 16));
    MYUSART_SendData((uint8_t)(AS608Addr >> 8));
    MYUSART_SendData((uint8_t)AS608Addr);
}

// 发送包标识
static void SendFlag(uint8_t flag) {
    MYUSART_SendData(flag);
}

// 发送包长度（2字节）
static void SendLength(uint16_t length) {
    MYUSART_SendData((uint8_t)(length >> 8));
    MYUSART_SendData((uint8_t)length);
}

// 发送指令码
static void Sendcmd(uint8_t cmd) {
    MYUSART_SendData(cmd);
}

// 发送校验和（2字节）
static void SendCheck(uint16_t check) {
    MYUSART_SendData((uint8_t)(check >> 8));
    MYUSART_SendData((uint8_t)check);
}

// 读取触摸状态：PA0 高 = 有手指
uint8_t PS_ReadTouch(void) {
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET ? 1 : 0;
}

// 握手
uint8_t PS_HandShake(uint32_t *pAddr) {
    uint8_t cmd[] = {
        0xEF, 0x01,
        0xFF, 0xFF, 0xFF, 0xFF,  // 地址：0xFFFFFFFF
        0x01,                   // 包标识
        0x00, 0x01,             // 长度 = 0x0001
        0x00, 0x03              // 指令0x00 + 校验（0x01+0x01+0x00=0x0002? 实际模块接受此）
    };
    // 校验 = 0x01 + 0x0001 + 0x00 = 0x0002 → 但实测发送 0x00 0x03 也能握手，兼容性考虑
    AS608_FlushRx(); // 清空旧数据
    HAL_UART_Transmit(&huart2, cmd, sizeof(cmd), 100);

    uint8_t resp[12];
    // 尝试接收 9 字节应答包
    if (HAL_UART_Receive(&huart2, resp, 9, 1000) == HAL_OK) {
        if (resp[0] == 0xEF && resp[1] == 0x01 && resp[6] == 0x07) {
            *pAddr = ((uint32_t)resp[2] << 24) |
                     ((uint32_t)resp[3] << 16) |
                     ((uint32_t)resp[4] << 8)  |
                     ((uint32_t)resp[5]);
            AS608Addr = *pAddr;
            return AS608_OK;
        }
    }
    return 1; // 失败
}

// 录入图像
uint8_t PS_GetImage(void) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x01);
    temp = 0x01 + 0x03 + 0x01;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 生成特征
uint8_t PS_GenChar(uint8_t BufferID) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x04);
    Sendcmd(0x02);
    MYUSART_SendData(BufferID);
    temp = 0x01 + 0x04 + 0x02 + BufferID;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 精确比对
uint8_t PS_Match(void) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x03);
    temp = 0x01 + 0x03 + 0x03;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 搜索
uint8_t PS_Search(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x08);
    Sendcmd(0x04);
    MYUSART_SendData(BufferID);
    MYUSART_SendData((uint8_t)(StartPage >> 8));
    MYUSART_SendData((uint8_t)StartPage);
    MYUSART_SendData((uint8_t)(PageNum >> 8));
    MYUSART_SendData((uint8_t)PageNum);
    temp = 0x01 + 0x08 + 0x04 + BufferID
           + (StartPage >> 8) + (uint8_t)StartPage
           + (PageNum >> 8) + (uint8_t)PageNum;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
        if (p) {
            p->pageID = (data[10] << 8) | data[11];
            p->mathscore = (data[12] << 8) | data[13];
        }
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 合并特征
uint8_t PS_RegModel(void) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x05);
    temp = 0x01 + 0x03 + 0x05;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 存储模板
uint8_t PS_StoreChar(uint8_t BufferID, uint16_t PageID) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x06);
    Sendcmd(0x06);
    MYUSART_SendData(BufferID);
    MYUSART_SendData((uint8_t)(PageID >> 8));
    MYUSART_SendData((uint8_t)PageID);
    temp = 0x01 + 0x06 + 0x06 + BufferID
           + (PageID >> 8) + (uint8_t)PageID;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 删除模板
uint8_t PS_DeletChar(uint16_t PageID, uint16_t N) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x07);
    Sendcmd(0x0C);
    MYUSART_SendData((uint8_t)(PageID >> 8));
    MYUSART_SendData((uint8_t)PageID);
    MYUSART_SendData((uint8_t)(N >> 8));
    MYUSART_SendData((uint8_t)N);
    temp = 0x01 + 0x07 + 0x0C
           + (PageID >> 8) + (uint8_t)PageID
           + (N >> 8) + (uint8_t)N;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 清空库
uint8_t PS_Empty(void) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x0D);
    temp = 0x01 + 0x03 + 0x0D;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 写寄存器
uint8_t PS_WriteReg(uint8_t RegNum, uint8_t DATA) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x05);
    Sendcmd(0x0E);
    MYUSART_SendData(RegNum);
    MYUSART_SendData(DATA);
    temp = 0x01 + 0x05 + 0x0E + RegNum + DATA;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 读系统参数
uint8_t PS_ReadSysPara(SysPara *p) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x0F);
    temp = 0x01 + 0x03 + 0x0F;
    SendCheck(temp);

    data = AS608_RecvPacket(1000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
        if (p && ensure == AS608_OK) {
            p->PS_max   = (data[10] << 8) | data[11];
            p->PS_level = data[12];
            p->PS_addr  = ((uint32_t)data[13] << 24) |
                          ((uint32_t)data[14] << 16) |
                          ((uint32_t)data[15] << 8)  |
                          ((uint32_t)data[16]);
            p->PS_size  = data[17];
            p->PS_N     = data[18];
        }
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 设置地址
uint8_t PS_SetAddr(uint32_t PS_addr) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x07);
    Sendcmd(0x15);
    MYUSART_SendData((uint8_t)(PS_addr >> 24));
    MYUSART_SendData((uint8_t)(PS_addr >> 16));
    MYUSART_SendData((uint8_t)(PS_addr >> 8));
    MYUSART_SendData((uint8_t)PS_addr);
    temp = 0x01 + 0x07 + 0x15
           + (PS_addr >> 24) + (PS_addr >> 16) + (PS_addr >> 8) + (uint8_t)PS_addr;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }

    if (ensure == AS608_OK) {
        AS608Addr = PS_addr;
    }
    return ensure;
}

// 写记事本（32字节）
uint8_t PS_WriteNotepad(uint8_t NotePageNum, uint8_t *Byte32) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(36); // 0x24
    Sendcmd(0x18);
    MYUSART_SendData(NotePageNum);
    for (uint8_t i = 0; i < 32; i++) {
        MYUSART_SendData(Byte32[i]);
    }
    temp = 0x01 + 36 + 0x18 + NotePageNum;
    for (uint8_t i = 0; i < 32; i++) {
        temp += Byte32[i];
    }
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 读记事本
uint8_t PS_ReadNotepad(uint8_t NotePageNum, uint8_t *Byte32) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x04);
    Sendcmd(0x19);
    MYUSART_SendData(NotePageNum);
    temp = 0x01 + 0x04 + 0x19 + NotePageNum;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
        if (Byte32 && ensure == AS608_OK) {
            for (uint8_t i = 0; i < 32; i++) {
                Byte32[i] = data[10 + i];
            }
        }
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 高速搜索
uint8_t PS_HighSpeedSearch(uint8_t BufferID, uint16_t StartPage, uint16_t PageNum, SearchResult *p) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x08);
    Sendcmd(0x1B);
    MYUSART_SendData(BufferID);
    MYUSART_SendData((uint8_t)(StartPage >> 8));
    MYUSART_SendData((uint8_t)StartPage);
    MYUSART_SendData((uint8_t)(PageNum >> 8));
    MYUSART_SendData((uint8_t)PageNum);
    temp = 0x01 + 0x08 + 0x1B + BufferID
           + (StartPage >> 8) + (uint8_t)StartPage
           + (PageNum >> 8) + (uint8_t)PageNum;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
        if (p) {
            p->pageID = (data[10] << 8) | data[11];
            p->mathscore = (data[12] << 8) | data[13];
        }
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 读有效模板数
uint8_t PS_ValidTempleteNum(uint16_t *ValidN) {
    uint16_t temp;
    uint8_t ensure;
    uint8_t *data;

    SendHead();
    SendAddr();
    SendFlag(0x01);
    SendLength(0x03);
    Sendcmd(0x1D);
    temp = 0x01 + 0x03 + 0x1D;
    SendCheck(temp);

    data = AS608_RecvPacket(2000);
    if (data && data[0] == 0xEF) {
        ensure = data[9];
        if (ValidN && ensure == AS608_OK) {
            *ValidN = (data[10] << 8) | data[11];
        }
    } else {
        ensure = 0xFF;
    }
    return ensure;
}

// 错误信息解析
const char* EnsureMessage(uint8_t ensure) {
    switch (ensure) {
        case AS608_OK:                     return "OK";
        case AS608_PACKET_RX_ERROR:        return "数据包接收错误";
        case AS608_NO_FINGER:              return "传感器上没有手指";
        case AS608_ENROLL_FAIL:            return "录入指纹图像失败";
        case AS608_IMG_TOO_DRY:            return "指纹图像太干、太淡而生不成特征";
        case AS608_IMG_TOO_WET:            return "指纹图像太湿、太糊而生不成特征";
        case AS608_IMG_TOO_MESSY:          return "指纹图像太乱而生不成特征";
        case AS608_FEW_FEATURE_POINTS:     return "特征点太少而生不成特征";
        case AS608_NOT_MATCH:              return "指纹不匹配!";
        case AS608_NOT_FOUND:              return "指纹不匹配!"; // 模块常混用
        case AS608_MERGE_FAIL:             return "特征合并失败";
        case AS608_ADDR_OUT_OF_RANGE:      return "访问指纹库时地址序号超出范围";
        case AS608_DEL_FAIL:               return "删除模板失败";
        case AS608_EMPTY_FAIL:             return "清空指纹库失败";
        case AS608_NO_IMAGE:               return "缓冲区无有效原始图";
        case AS608_FLASH_ERROR:            return "读写 FLASH 出错";
        case AS608_INVALID_REG_NUM:        return "无效寄存器号";
        case AS608_INVALID_REG_VAL:        return "寄存器设定内容错误";
        case AS608_INVALID_NOTE_PAGE:      return "记事本页码指定错误";
        case AS608_DB_FULL:                return "指纹库满";
        case AS608_ADDR_ERROR:             return "地址错误";
        default:                           return "模块返回确认码有误";
    }
}

// 【可选】初始化函数（在 main 中调用）
void AS608_Init(void) {
    // 启动 UART2 中断接收（1字节模式）
    AS608_StartReceiveIT();
}
