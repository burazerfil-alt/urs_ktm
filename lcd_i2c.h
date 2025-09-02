#ifndef __LCD_I2C_H__       // Ako __LCD_I2C_H__ nije definiran...
#define __LCD_I2C_H__       // ...definiraj ga (sprječava višestruko uključivanje ovog headera)

// Uključuje osnovne definicije za STM32F4 i HAL biblioteku.
// Potrebno da bi koristili I2C tipove i funkcije (npr. I2C_HandleTypeDef).
#include "stm32f4xx_hal.h"  

// Inicijalizacija LCD-a preko I2C-a
// - prima pokazivač na I2C handler (npr. &hi2c1 iz CubeMX-a)
// - I2C adresa LCD modula (npr. 0x27)
// - broj stupaca (cols = 16) i redova (rows = 2)
void lcd_i2c_init(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t cols, uint8_t rows);

// Briše cijeli zaslon i vraća kursor na početak
void lcd_i2c_clear(void);

// Postavlja kursor na određeni položaj
// col = stupac (0–15), row = red (0–1 kod 16x2 LCD-a)
void lcd_i2c_set_cursor(uint8_t col, uint8_t row);

// Ispisuje string (niz znakova) na LCD od trenutne pozicije kursora
void lcd_i2c_print(char *str);

#endif  // završetak zaštite od višestrukog uključivanja
