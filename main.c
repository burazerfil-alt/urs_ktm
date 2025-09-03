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

// -Funkcije za signalizaciju (LED i buzzer) 

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
    lcd_i2c_clear();              // obriši ekran
    lcd_i2c_set_cursor(0, 0);     // kursor na prvi red, prvi stupac
    lcd_i2c_print("Upisi lozinku:"); // ispiši poruku
    lcd_i2c_set_cursor(0, 1);     // kursor na drugi red
    lcd_i2c_print("                "); // ispiši praznine da očisti red
}

int main(void)
{
    HAL_Init();          // Inicijalizacija HAL biblioteke i Systick timera
    SystemClock_Config();// Konfiguracija sistemskog takta (definirana niže)
    MX_GPIO_Init();      // Inicijalizacija GPIO pinova (CubeMX generira funkciju)
    MX_I2C1_Init();      // Inicijalizacija I2C1 (CubeMX generira funkciju)

    // Pokreni LCD (koristimo I2C1, adresa 0x27, LCD 16x2)
    lcd_i2c_init(&hi2c1, 0x27, 16, 2); // inicijalizacija LCD-a
    lcd_i2c_clear();                   // obriši ekran
    lcd_i2c_set_cursor(0, 0);          // kursor na prvi red, prva pozicija
    lcd_i2c_print("Upisi lozinku:");   // ispiši početnu poruku
    lcd_i2c_set_cursor(0, 1);          // kursor na drugi red

    Keypad_Init();       // Inicijalizacija tipkovnice (GPIO pinovi)

    // Varijable za stanje sustava
    char input[PASSWORD_LEN + 1] = {0}; // buffer za unos lozinke (4 znaka + terminator '\0')
    int idx = 0;                        // trenutačni indeks unosa (0–4)
    int unlocked = 0;                   // status otključano (1) ili zaključano (0)
    int tocno = 0;                      // zastavica: zadnja lozinka bila točna
    int fail_count = 0;                 // broj uzastopnih pogrešnih unosa
    int locked = 0;                     // zastavica: 1 = sustav blokiran nakon 3 greške

    // Na početku isključi LED i buzzer
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

    // Beskonačna glavna petlja
    while (1)
    {
        // Provjera hardverskog tipkala (PC13, aktivno na niskom nivou)
        if (HAL_GPIO_ReadPin(RESET_GPIO_Port, RESET_Pin) == GPIO_PIN_RESET) {
            HAL_Delay(80); // debounce (odstranjivanje smetnji pri pritisku)
            reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked); // resetiraj stanje
            while(HAL_GPIO_ReadPin(RESET_GPIO_Port, RESET_Pin) == GPIO_PIN_RESET); // čekaj da tipkalo bude pušteno
            HAL_Delay(80); // dodatni debounce
        }

        // Čitanje tipke s tipkovnice
        char key = Keypad_GetKey(); // funkcija vraća znak ili 0 ako ništa nije pritisnuto
        if (key) {                  // ako je pritisnuta tipka
            // Ako je sustav zaključan zbog 3 greške
            if (locked) {
                if (key == '*') { // samo '*' resetira
                    reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
                }
                HAL_Delay(200); // debounce
                continue;       // preskoči ostatak jer je zaključano
            }

            // Ako je '*' → soft reset
            if (key == '*') {
                reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
            }
            // Ako je '#' → ulazak u mod promjene lozinke
            else if (key == '#') {
                changeMode = 1;         // postavi zastavicu promjene lozinke
                idx = 0;                // reset indeksa unosa
                input[0] = '\0';        // prazan unos
                lcd_i2c_clear();        // očisti LCD
                lcd_i2c_set_cursor(0, 0);
                lcd_i2c_print("Nova lozinka:"); // poruka korisniku
                lcd_i2c_set_cursor(0, 1);
            }
            // Inače unosimo lozinku znak po znak
            else if (idx < PASSWORD_LEN) {
                input[idx++] = key;         // spremi znak u buffer
                input[idx] = '\0';          // dodaj terminator stringa
                lcd_i2c_set_cursor(0, 1);   // postavi kursor na drugi red
                lcd_i2c_print(input);       // ispiši trenutačni unos
                for (int i = idx; i < 16; i++) lcd_i2c_print(" "); // ostatak reda ispuni prazninama

                // Ako je uneseno svih 4 znaka
                if (idx == PASSWORD_LEN) {
                    if (changeMode) {
                        // U modu promjene lozinke
                        strcpy(password, input);   // spremi novu lozinku
                        changeMode = 0;            // izađi iz moda
                        lcd_i2c_clear();
                        lcd_i2c_print("Lozinka promj."); // obavijest korisniku
                        buzzer_beep(1, 200);       // zvučni signal
                        HAL_Delay(800);            // čekaj malo
                        reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked); // resetiraj stanje
                    } else {
                        // Provjera unosa lozinke
                        if (strcmp(input, password) == 0) {
                            // Ako je točno
                            lcd_i2c_clear();
                            lcd_i2c_print("Tocna lozinka!");
                            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // LED ON
                            buzzer_beep(1, 200);                                     // beep
                            tocno = 1;      // označi da je zadnji unos bio točan
                            unlocked = 1;   // status otključano
                            fail_count = 0; // reset broja grešaka
                        } else {
                            // Ako je pogrešno
                            fail_count++;               // povećaj broj grešaka
                            lcd_i2c_clear();
                            lcd_i2c_print("Pogresna lozinka!");
                            led_blink(2, 200);          // LED blink dvaput
                            buzzer_beep(2, 120);        // buzzer beep dvaput
                            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // LED OFF
                            unlocked = 0;
                            tocno = 0;
                            idx = 0;         // reset unosa
                            input[0] = '\0';

                            if (fail_count >= 3) {
                                // Zaključaj sustav
                                locked = 1;
                                lcd_i2c_clear();
                                lcd_i2c_set_cursor(0, 0);
                                lcd_i2c_print("Zakljucano!");
                                lcd_i2c_set_cursor(0, 1);
                                lcd_i2c_print("Reset na * ili tipk.");
                            } else {
                                // Vrati na početnu poruku
                                lcd_i2c_set_cursor(0, 0);
                                lcd_i2c_print("Upisi lozinku:");
                                lcd_i2c_set_cursor(0, 1);
                                lcd_i2c_print("                ");
                            }
                        }
                    }
                }
            }
            HAL_Delay(200); // debounce
        }

        // Ako je zadnja lozinka bila točna → LED stalno svijetli
        if (tocno) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        }
    }
}

// --- Konfiguracija sistemskog takta (HSI, bez PLL-a) ---
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};  // Struktura za konfiguraciju oscilatora
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};  // Struktura za konfiguraciju takta sabirnica

  __HAL_RCC_PWR_CLK_ENABLE();                                      // Omogući PWR clock
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);   // Postavi naponsko skaliranje

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;       // Odaberi HSI oscilator
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;                         // Uključi HSI
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT; // Kalibracija HSI
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;                   // Ne koristimo PLL
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)             // Primijeni postavke
  {
    Error_Handler();                                               // Ako ne uspije, idi u Error_Handler
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2; // Konfiguriraj sve sabirnice
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;           // SYSCLK iz HSI
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;               // AHB = SYSCLK / 1
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                // APB1 = HCLK / 1
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;                // APB2 = HCLK / 1

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) // Primijeni clock konfiguraciju
  {
    Error_Handler();                                               // Ako ne uspije, idi u Error_Handler
  }
}

// --- Funkcija za greške ---
void Error_Handler(void)
{
  __disable_irq(); // Onemogući sve prekide
  while (1) {      // Beskonačna petlja (fail-safe stanje)
  }
}
