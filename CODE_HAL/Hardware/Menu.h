#ifndef __MENU_H__
#define __MENU_H__

#include <stdint.h>

#define PASSWORD_LEN 4

void Menu_Show_UI(void);
void Lock_UI(void);
void Menu_UI(void);
void Lock_Page(void);
uint8_t First_Page(void);
uint8_t UnLock_Page(void);
uint8_t OldPassword_Page(void);
uint8_t NewPassword_Page(void);
uint8_t Add_Card_Page(void);
uint8_t Delete_Card_Page(void);
uint8_t Check_Password(void);
uint8_t Add_Fingerprint_Page(void);
uint8_t Delete_Fingerprint_Page(void);


#endif

