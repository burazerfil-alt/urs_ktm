#include "keypad.h"   // Uključujemo header fajl s deklaracijama i HAL funkcijama

// Definicija pinova za redove i kolone tipkovnice
// PA0, PA1, PA8, PA9 -> redovi
// PA4, PA5, PA6, PA7 -> kolone
static const uint16_t rowPins[4] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_8, GPIO_PIN_9};
static const uint16_t colPins[4] = {GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7};
#define KEYPAD_GPIO GPIOA   // Tipkovnica je spojena na PORTA

// Matrica tipkovnice 4x4 – mapiranje pritisnute tipke na znak
static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

void Keypad_Init(void) {

}

char Keypad_GetKey(void) {
    // Petlja prolazi kroz svaki red
    for (int row = 0; row < 4; row++) {
        // Svi redovi se prvo postave u LOW
        for (int r = 0; r < 4; r++)
            HAL_GPIO_WritePin(KEYPAD_GPIO, rowPins[r], GPIO_PIN_RESET);

        // Trenutni red se postavlja u HIGH
        HAL_GPIO_WritePin(KEYPAD_GPIO, rowPins[row], GPIO_PIN_SET);

        // Kratko čekanje da se signal stabilizira (debounce)
        HAL_Delay(1);

        // Provjeravamo sve kolone
        for (int col = 0; col < 4; col++) {
            // Ako je neka kolona HIGH → znači da je tipka pritisnuta
            if (HAL_GPIO_ReadPin(KEYPAD_GPIO, colPins[col]) == GPIO_PIN_SET) {
                // Čekamo dok korisnik ne otpusti tipku (sprječava dupli unos)
                while(HAL_GPIO_ReadPin(KEYPAD_GPIO, colPins[col]) == GPIO_PIN_SET);

                // Vraćamo znak tipke iz matrice keymap
                return keymap[row][col];
            }
        }
    }

    // Ako ništa nije pritisnuto, vraća 0
    return 0;
}
