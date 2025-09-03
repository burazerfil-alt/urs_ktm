#include "lcd_i2c.h"     // Uključuje header datoteku s deklaracijama funkcija za LCD
#include "main.h"        // Uključuje HAL definicije i globalne varijable iz CubeMX-a
#include <string.h>      // Biblioteka za rad sa stringovima (strlen, strcpy...), zgodno za ispis teksta

// --- Staticke varijable za čuvanje stanja LCD-a ---
static I2C_HandleTypeDef *lcd_i2c; // Pokazivač na I2C handler (definiran u main.c) – HAL ga koristi za komunikaciju
static uint8_t lcd_addr;           // I2C adresa LCD modula (obično 0x27 ili 0x3F)
static uint8_t lcd_cols;           // Broj stupaca (npr. 16 ili 20)
static uint8_t lcd_rows;           // Broj redaka (npr. 2 ili 4)

// --- Definicije kontrolnih bita prema PCF8574 expanderu ---
#define LCD_BACKLIGHT 0x08   // Bit koji uključuje pozadinsko svjetlo LCD-a
#define LCD_ENABLE    0x04   // Bit povezan na E pin LCD-a (Enable) – kad je 1, LCD očitava podatke
#define LCD_COMMAND   0      // RS=0 → označava da šaljemo naredbu (command)
#define LCD_DATA      1      // RS=1 → označava da šaljemo podatke (tekst, znakove)

// --- Funkcija za slanje naredbi LCD-u ---
static void lcd_send_cmd(uint8_t cmd) {
    uint8_t data_u, data_l;  // Gornja i donja polovica bajta (high i low nibble)
    uint8_t data_t[4];       // Buffer od 4 bajta koji se šalje preko I2C-a

    data_u = (cmd & 0xF0);         // Uzmi gornjih 4 bita naredbe (high nibble)
    data_l = ((cmd << 4) & 0xF0);  // Pomakni donjih 4 bita u gornji položaj (low nibble → high)

    // Slanje gornjih 4 bita:
    data_t[0] = data_u | LCD_BACKLIGHT | LCD_ENABLE; // Postavi podatke + pozadinsko svjetlo + E=1
    data_t[1] = data_u | LCD_BACKLIGHT;              // Spusti E=0 → LCD registrira podatke

    // Slanje donjih 4 bita:
    data_t[2] = data_l | LCD_BACKLIGHT | LCD_ENABLE; // Postavi donjih 4 bita + E=1
    data_t[3] = data_l | LCD_BACKLIGHT;              // Spusti E=0 → LCD registrira podatke

    // Pošalji svih 4 faza preko I2C-a
    HAL_I2C_Master_Transmit(lcd_i2c, lcd_addr << 1, data_t, 4, 100);
}

// --- Funkcija za slanje podataka (znakova) LCD-u ---
static void lcd_send_data(uint8_t data) {
    uint8_t data_u, data_l;  // Gornjih i donjih 4 bita znaka
    uint8_t data_t[4];       // Buffer od 4 bajta za prijenos

    data_u = (data & 0xF0);        // Uzmi gornjih 4 bita znaka
    data_l = ((data << 4) & 0xF0); // Pomakni donjih 4 bita u gornji položaj

    // Slanje gornjih 4 bita znaka (RS=1 jer šaljemo podatke, ne naredbe)
    data_t[0] = data_u | LCD_BACKLIGHT | LCD_ENABLE | LCD_DATA;
    data_t[1] = data_u | LCD_BACKLIGHT | LCD_DATA;   // E=0 → potvrda

    // Slanje donjih 4 bita znaka
    data_t[2] = data_l | LCD_BACKLIGHT | LCD_ENABLE | LCD_DATA;
    data_t[3] = data_l | LCD_BACKLIGHT | LCD_DATA;   // E=0 → potvrda

    // Pošalji sve 4 faze preko I2C-a
    HAL_I2C_Master_Transmit(lcd_i2c, lcd_addr << 1, data_t, 4, 100);
}

// --- Funkcija za inicijalizaciju LCD-a ---
void lcd_i2c_init(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t cols, uint8_t rows) {
    lcd_i2c = hi2c;   // Spremi I2C handler
    lcd_addr = addr;  // Spremi I2C adresu modula
    lcd_cols = cols;  // Spremi broj stupaca
    lcd_rows = rows;  // Spremi broj redaka
    HAL_Delay(50);    // Pričekaj 50 ms da se LCD uključi

    // Inicijalizacijska sekvenca prema datasheetu za HD44780
    lcd_send_cmd(0x30);  // Force 8-bit mode
    HAL_Delay(5);        // Pričekaj
    lcd_send_cmd(0x30);  // Ponovi
    HAL_Delay(1);        // Kratko čekanje
    lcd_send_cmd(0x30);  // Još jednom
    HAL_Delay(10);       // Pričekaj
    lcd_send_cmd(0x20);  // Sada prebaci u 4-bitni način rada
    HAL_Delay(10);

    // Standardne postavke nakon prelaska u 4-bit mode
    lcd_send_cmd(0x28);  // Function set: 4-bit, 2 linije, font 5x8
    HAL_Delay(1);
    lcd_send_cmd(0x08);  // Display OFF
    HAL_Delay(1);
    lcd_send_cmd(0x01);  // Clear display
    HAL_Delay(2);
    lcd_send_cmd(0x06);  // Entry mode: automatski pomak kursora udesno
    HAL_Delay(1);
    lcd_send_cmd(0x0C);  // Display ON, cursor OFF
}

// --- Očisti cijeli LCD ---
void lcd_i2c_clear(void) {
    lcd_send_cmd(0x01); // Naredba za brisanje ekrana
    HAL_Delay(2);       // Brisanje traje duže → čekamo 2 ms
}

// --- Postavi kursor na određenu poziciju (col, row) ---
void lcd_i2c_set_cursor(uint8_t col, uint8_t row) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54}; // DDRAM adrese početaka redova
    lcd_send_cmd(0x80 | (col + row_offsets[row]));    // 0x80 = naredba za set DDRAM address
}

// --- Ispis stringa na LCD ---
void lcd_i2c_print(char *str) {
    while (*str) {             // Dok string ne dođe do '\0'
        lcd_send_data(*str++); // Šaljemo znak po znak i pomičemo pointer
    }
}
