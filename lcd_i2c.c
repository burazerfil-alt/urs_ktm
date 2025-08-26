#include "lcd_i2c.h"     // Header za LCD funkcije (deklaracije)
#include "main.h"        // Potrebno za HAL i definicije
#include <string.h>      // Za rad sa stringovima (nije nužno ovdje, ali zgodno za print)

// --- Staticke varijable koje čuvaju informacije o LCD-u ---
static I2C_HandleTypeDef *lcd_i2c; // pokazivač na I2C handler (iz CubeMX-a)
static uint8_t lcd_addr;           // I2C adresa LCD-a
static uint8_t lcd_cols;           // broj stupaca (npr. 16)
static uint8_t lcd_rows;           // broj redaka (npr. 2)

// --- Definicije kontrolnih bita prema PCF8574 expanderu ---
#define LCD_BACKLIGHT 0x08   // uključuje pozadinsko osvjetljenje
#define LCD_ENABLE    0x04   // Enable pin LCD-a
#define LCD_COMMAND   0      // oznaka da šaljemo naredbu
#define LCD_DATA      1      // oznaka da šaljemo podatke (znakove)

// --- Funkcija za slanje naredbi LCD-u (privatna) ---
static void lcd_send_cmd(uint8_t cmd) {
    uint8_t data_u, data_l;  // gornja i donja polovica bajta
    uint8_t data_t[4];       // buffer za prijenos preko I2C-a

    data_u = (cmd & 0xf0);          // uzmi gornjih 4 bita
    data_l = ((cmd << 4) & 0xf0);   // pomakni dolje donjih 4 bita

    // Prvo gornjih 4 bita
    data_t[0] = data_u | LCD_BACKLIGHT | LCD_ENABLE; // E=1
    data_t[1] = data_u | LCD_BACKLIGHT;              // E=0

    // Onda donjih 4 bita
    data_t[2] = data_l | LCD_BACKLIGHT | LCD_ENABLE; // E=1
    data_t[3] = data_l | LCD_BACKLIGHT;              // E=0

    // Pošalji sve 4 sekvence preko I2C-a
    HAL_I2C_Master_Transmit(lcd_i2c, lcd_addr<<1, data_t, 4, 100);
}

// --- Funkcija za slanje znakova (podataka) LCD-u (privatna) ---
static void lcd_send_data(uint8_t data) {
    uint8_t data_u, data_l;
    uint8_t data_t[4];

    data_u = (data & 0xf0);         // gornjih 4 bita
    data_l = ((data << 4) & 0xf0);  // donjih 4 bita

    // Prvo gornji dio (s DATA bitom)
    data_t[0] = data_u | LCD_BACKLIGHT | LCD_ENABLE | LCD_DATA;
    data_t[1] = data_u | LCD_BACKLIGHT | LCD_DATA;

    // Zatim donji dio
    data_t[2] = data_l | LCD_BACKLIGHT | LCD_ENABLE | LCD_DATA;
    data_t[3] = data_l | LCD_BACKLIGHT | LCD_DATA;

    // Pošalji preko I2C
    HAL_I2C_Master_Transmit(lcd_i2c, lcd_addr<<1, data_t, 4, 100);
}

// --- Inicijalizacija LCD-a ---
void lcd_i2c_init(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t cols, uint8_t rows) {
    lcd_i2c = hi2c;   // spremi pointer na I2C
    lcd_addr = addr;  // I2C adresa (obično 0x27 ili 0x3F)
    lcd_cols = cols;  // broj stupaca
    lcd_rows = rows;  // broj redaka
    HAL_Delay(50);    // pričekaj 50 ms da se LCD probudi

    // Sekvenca inicijalizacije prema datasheetu za HD44780
    lcd_send_cmd(0x30);
    HAL_Delay(5);
    lcd_send_cmd(0x30);
    HAL_Delay(1);
    lcd_send_cmd(0x30);
    HAL_Delay(10);
    lcd_send_cmd(0x20);  // Postavi 4-bitni način rada
    HAL_Delay(10);

    lcd_send_cmd(0x28);  // Function set: 4-bit, 2 linije, 5x8 font
    HAL_Delay(1);
    lcd_send_cmd(0x08);  // Display OFF
    HAL_Delay(1);
    lcd_send_cmd(0x01);  // Clear display
    HAL_Delay(2);
    lcd_send_cmd(0x06);  // Entry mode set: automatski pomak kursora desno
    HAL_Delay(1);
    lcd_send_cmd(0x0C);  // Display ON, cursor OFF
}

// --- Očisti LCD ---
void lcd_i2c_clear(void) {
    lcd_send_cmd(0x01); // naredba za brisanje
    HAL_Delay(2);       // brisanje traje malo duže
}

// --- Postavi kursor na određenu poziciju ---
void lcd_i2c_set_cursor(uint8_t col, uint8_t row) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54}; // adrese početaka redova
    lcd_send_cmd(0x80 | (col + row_offsets[row]));    // naredba: set DDRAM addr
}

// --- Ispis stringa na LCD ---
void lcd_i2c_print(char *str) {
    while (*str) {             // dok string nije gotov
        lcd_send_data(*str++); // pošalji znak po znak
    }
}
