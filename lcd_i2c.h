#ifndef __LCD_I2C_H__       // Ako __LCD_I2C_H__ nije definiran...
#define __LCD_I2C_H__       // ...definiraj ga (sprječava višestruko uključivanje ovog headera)

// Uključuje osnovne definicije za STM32F4 i HAL biblioteku.
// Potrebno da bi koristili I2C tipove i funkcije (npr. I2C_HandleTypeDef).
#include "stm32f4xx_hal.h"  

// --- Prototipovi funkcija za rad s LCD-om preko I2C-a ---

// Inicijalizacija LCD-a preko I2C-a
// Parametri:
//   hi2c  → pokazivač na I2C handler (npr. &hi2c1 iz CubeMX-a)
//   addr  → I2C adresa LCD modula (najčešće 0x27 ili 0x3F)
//   cols  → broj stupaca LCD-a (npr. 16 ili 20)
//   rows  → broj redova LCD-a (npr. 2 ili 4)
void lcd_i2c_init(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t cols, uint8_t rows);

// Briše cijeli zaslon LCD-a i vraća kursor na početnu poziciju (0,0).
void lcd_i2c_clear(void);

// Postavlja kursor na određeni položaj na LCD-u.
// Parametri:
//   col → stupac (0–15 kod 16x2 LCD-a)
//   row → red (0–1 kod 16x2 LCD-a, ili 0–3 kod 20x4 LCD-a)
void lcd_i2c_set_cursor(uint8_t col, uint8_t row);

// Ispisuje string (niz znakova) na LCD-u počevši od trenutne pozicije kursora.
// Parametar:
//   str → pointer na char niz (klasični C string)
void lcd_i2c_print(char *str);

#endif  // završetak zaštite od višestrukog uključivanja
