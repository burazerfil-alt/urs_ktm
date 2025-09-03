#include "keypad.h"   // Uključujemo header file s deklaracijama i HAL funkcijama

// - Definicija pinova za redove i kolone tipkovnice 
// Tipkovnica 4x4 spojena na GPIOA
// Redovi: PA0, PA1, PA8, PA9
// Kolone: PA4, PA5, PA6, PA7
static const uint16_t rowPins[4] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_8, GPIO_PIN_9};
static const uint16_t colPins[4] = {GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7};
#define KEYPAD_GPIO GPIOA   // cijela tipkovnica je na portu A

// - Matrica tipkovnice 4x4 
// Mapira kombinaciju (red, kolona) na određeni znak
static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// - Inicijalizacija tipkovnice 
// (GPIO pinovi se podešavaju u CubeMX-u kao izlazi/ulazi, pa ovdje ništa ne radimo)
void Keypad_Init(void) {
    // Ova funkcija ostaje prazna jer su pinovi konfigurirani u MX_GPIO_Init().
    // Ako želiš ručno, ovdje bi koristio HAL_GPIO_Init() i podesio row kao output, col kao input.
}

// - Čitanje pritisnute tipke 
// Vraća znak tipke ili 0 ako ništa nije pritisnuto
char Keypad_GetKey(void) {
    // Prolazimo kroz sve redove jedan po jedan
    for (int row = 0; row < 4; row++) {
        // 1) Resetiraj sve redove na LOW (0)
        for (int r = 0; r < 4; r++)
            HAL_GPIO_WritePin(KEYPAD_GPIO, rowPins[r], GPIO_PIN_RESET);

        // 2) Aktiviraj samo trenutni red (postavi HIGH = 1)
        HAL_GPIO_WritePin(KEYPAD_GPIO, rowPins[row], GPIO_PIN_SET);

        // 3) Kratko čekanje da se signal stabilizira (debounce)
        HAL_Delay(1);

        // 4) Provjeravamo sve kolone
        for (int col = 0; col < 4; col++) {
            // Ako je neka kolona HIGH → tipka pritisnuta u tom retku i koloni
            if (HAL_GPIO_ReadPin(KEYPAD_GPIO, colPins[col]) == GPIO_PIN_SET) {
                // Čekaj dok korisnik ne otpusti tipku (sprječava višestruki unos)
                while (HAL_GPIO_ReadPin(KEYPAD_GPIO, colPins[col]) == GPIO_PIN_SET);

                // Vrati znak tipke iz matrice keymap
                return keymap[row][col];
            }
        }
    }

    // Ako nijedna tipka nije pritisnuta → vrati 0
    return 0;
}

