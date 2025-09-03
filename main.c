#include "main.h"          // Uključuje osnovne definicije projekta (HAL, tipovi, prototipovi koje generira CubeMX)
#include "i2c.h"           // Uključuje konfiguraciju i funkcije za I2C perif. (CubeMX generira i2c.c i i2c.h)
#include "gpio.h"          // Uključuje konfiguraciju i funkcije za GPIO pinove (CubeMX generira gpio.c i gpio.h)
#include "lcd_i2c.h"       // Uključuje našu LCD biblioteku (inicijalizacija, ispis teksta, pomicanje kursora)
#include "keypad.h"        // Uključuje našu biblioteku za tipkovnicu 4x4 (inicijalizacija i čitanje tipke)
#include <string.h>        // Standardna C biblioteka za rad sa stringovima (strcmp, strcpy, strcat, strlen...)

#define PASSWORD_LEN  4     // Definiramo konstantu: duljina lozinke je 4 znaka

// Globalne varijable za logiku brave
char password[PASSWORD_LEN + 1] = "1234";  // Polje od 5 bajtova (4 znaka + '\0'), inicijalno "1234"
int changeMode = 0;                        // Zastavica: 0 = normalno, 1 = mod promjene lozinke

// Pinovi za LED i buzzer (definirani u CubeMX-u kao GPIO Output)
#define LED_GPIO_Port    GPIOA        // LED se nalazi na PORT A
#define LED_Pin          GPIO_PIN_11  // LED spojen na PA11
#define BUZZER_GPIO_Port GPIOA        // Buzzer na PORT A
#define BUZZER_Pin       GPIO_PIN_12  // Buzzer spojen na PA12

// Tipkalo za reset (definirano u CubeMX-u kao GPIO Input s Pull-Up otpornikom)
// Fizički tipkalo spaja pin PC13 na GND kad se pritisne
#define RESET_GPIO_Port  GPIOC
#define RESET_Pin        GPIO_PIN_13

// Handle za I2C (objekt koji koristi HAL za upravljanje I2C1 perif.)
extern I2C_HandleTypeDef hi2c1; // Definiran u i2c.c od CubeMX-a

// --- Funkcije za signalizaciju (LED i buzzer) ---

// Funkcija koja trepće LED-icom n puta
// Parametri: n = broj treptaja, delay_ms = trajanje ON i OFF stanja u milisekundama
void led_blink(int n, int delay_ms) {
    for (int i = 0; i < n; i++) {                                 // ponavljaj n puta
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   // LED ON (logička 1)
        HAL_Delay(delay_ms);                                       // čekaj delay_ms ms
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // LED OFF (logička 0)
        HAL_Delay(delay_ms);                                       // čekaj delay_ms ms
    }
}

// Funkcija koja daje zvučni signal buzzerom n puta
// Parametri: n = broj beepova, delay_ms = trajanje zvuka (ON) u milisekundama
void buzzer_beep(int n, int delay_ms) {
    for (int i = 0; i < n; i++) {                                      // ponavljaj n puta
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET); // Uključi buzzer
        HAL_Delay(delay_ms);                                           // čekaj dok traje zvuk
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);// Isključi buzzer
        HAL_Delay(100);                                                // kratka pauza 100 ms
    }
}

// Funkcija koja resetira stanje sustava i vrati ekran na početnu poruku
// Parametri su pokazivači na varijable stanja (unos, indeks, unlocked, tocno, fail_count, locked)
void reset_unlock(char *input, int *idx, int *unlocked, int *tocno, int *fail_count, int *locked) {
    *idx = 0;                     // reset indeksa unosa na početak
    input[0] = '\0';              // postavi prazan string (nulti znak = terminator)
    *unlocked = 0;                // postavi status "zaključano"
    *tocno = 0;                   // zadnji unos nije točan
    *fail_count = 0;              // reset broja pogrešnih pokušaja
    *locked = 0;                  // sustav nije zaključan
    changeMode = 0;               // izađi iz moda promjene lozinke

    // Isključi LED i buzzer
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

    // Reset LCD ekrana na početnu poruku
    lc
