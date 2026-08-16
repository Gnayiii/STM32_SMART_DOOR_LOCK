#ifndef __FINGERPRINT_FLASH_H
#define __FINGERPRINT_FLASH_H

#include <stdint.h>

#ifndef FINGERPRINT_FLASH_PAGE_ADDR
// （page56）
#define FINGERPRINT_FLASH_PAGE_ADDR  0x0800E000
#endif

#define FINGERPRINT_SLOT_COUNT 4
#define FINGERPRINT_EMPTY_SLOT 0xFFFFU

uint8_t FingerprintFlash_Add(uint16_t id);
uint8_t FingerprintFlash_ReadAll(uint16_t *ids, uint8_t max);
uint8_t FingerprintFlash_ClearAll(void);
uint8_t FingerprintFlash_AddSlot(uint8_t slot);
uint8_t FingerprintFlash_ReadSlots(uint8_t *slots, uint8_t max_slots); // 返回实际数量（1..4）
uint8_t FingerprintFlash_Count(void);
uint8_t FingerprintFlash_RemoveSlot(uint8_t slot);

#endif 
