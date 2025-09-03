#ifndef __KEYPAD_H__       // Ako __KEYPAD_H__ nije već definiran...
#define __KEYPAD_H__       // ...definiraj ga (štiti od višestrukog uključivanja)

// Potrebno zbog tipova i funkcija kao što su GPIO_TypeDef, HAL_GPIO_ReadPin, HAL_GPIO_WritePin itd.
#include "stm32f4xx_hal.h"

// Prototipovi funkcija za tipkovnicu 4x4 
//
// Keypad_Init()
// - Inicijalizira GPIO pinove za tipkovnicu (postavlja redove kao izlaz, stupce kao ulaz s pull-up)
//
// Keypad_GetKey()
// - Provjerava je li neka tipka pritisnuta
// - Ako je,vraća char ('0'–'9', 'A'–'D', '*' ili '#')
// - Ako nije pritisnuto ništa, vraća 0 (null karakter)
void Keypad_Init(void);
char Keypad_GetKey(void);

#endif // __KEYPAD_H__   // završetak zaštite od višestrukog uključivanja
