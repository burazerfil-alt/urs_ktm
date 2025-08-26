#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include "stm32f4xx_hal.h"

void Keypad_Init(void);
char Keypad_GetKey(void);

#endif
