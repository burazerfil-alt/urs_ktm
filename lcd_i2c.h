#ifndef __LCD_I2C_H__
#define __LCD_I2C_H__

#include "stm32f4xx_hal.h"

void lcd_i2c_init(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t cols, uint8_t rows);
void lcd_i2c_clear(void);
void lcd_i2c_set_cursor(uint8_t col, uint8_t row);
void lcd_i2c_print(char *str);

#endif
