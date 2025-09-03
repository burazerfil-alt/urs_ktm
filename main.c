#include "main.h"          // Osnovne definicije za projekt (HAL, tipovi, prototipovi funkcija iz CubeMX-a)
#include "i2c.h"           // Inicijalizacija i rukovanje I2C periferiom (CubeMX generira)
#include "gpio.h"          // Inicijalizacija i rukovanje GPIO pinovima
#include "lcd_i2c.h"       // Naša I2C LCD biblioteka (funkcije: init, clear, print, cursor)
#include "keypad.h"        // Naša tipkovnica 4x4 (Keypad_Init, Keypad_GetKey)
#include <string.h>        // Standardna C biblioteka za rad sa stringovima (strcmp, strcpy...)

#define PASSWORD_LEN  4     // Duljina lozinke (4 znaka)

// Globalne varijable za logiku brave
char password[PASSWORD_LEN + 1] = "1234";  // Trenutna lozinka (fiksna, +1 za '\0')
int changeMode = 0;                        // 1 = aktivan mod promjene lozinke

// Pinovi za LED i buzzer (definirani u CubeMX-u)
#define LED_GPIO_Port    GPIOA
#define LED_Pin          GPIO_PIN_11
#define BUZZER_GPIO_Port GPIOA
#define BUZZER_Pin       GPIO_PIN_12

// Tipkalo za reset (u CubeMX: PC13 kao Input s Pull-Up; tipkalo spaja na GND)
#define RESET_GPIO_Port  GPIOC
#define RESET_Pin        GPIO_PIN_13

// Handle za I2C (definiran u i2c.c koje generira CubeMX)
extern I2C_HandleTypeDef hi2c1;

// --- Funkcije za signalizaciju (LED i buzzer) ---

// Trepne LED-icom n puta, svako trajanje delay_ms
void led_blink(int n, int delay_ms) {
    for (int i = 0; i < n; i++) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   // LED ON
        HAL_Delay(delay_ms);                                       // čekaj
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // LED OFF
        HAL_Delay(delay_ms);                                       // čekaj
    }
}

// Pusti beep na buzzeru n puta, svaki beep delay_ms + pauza 100ms
void buzzer_beep(int n, int delay_ms) {
    for (int i = 0; i < n; i++) {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET); // Buzzer ON
        HAL_Delay(delay_ms);                                           // trajanje tona
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);// Buzzer OFF
        HAL_Delay(100);                                                // kratka pauza
    }
}

// Resetira stanje sustava i LCD ekran na početnu poruku
// input      -> buffer za unos lozinke
// idx        -> indeks sljedeće pozicije u inputu
// unlocked   -> status brave (0 = zaključana, 1 = otključana)
// tocno      -> zadnji unos bio ispravan
// fail_count -> broj pogrešnih pokušaja
// locked     -> zastavica za blokadu nakon 3 greške
void reset_unlock(char *input, int *idx, int *unlocked, int *tocno, int *fail_count, int *locked) {
    *idx = 0;                     // vrati unos na početak
    input[0] = '\0';              // prazni string
    *unlocked = 0;                // zaključano
    *tocno = 0;                   // nema zadnjeg točnog unosa
    *fail_count = 0;              // reset broja grešaka
    *locked = 0;                  // nije blokirano
    changeMode = 0;               // izađi iz moda promjene lozinke

    // Ugasi signalizaciju
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

    // Reset LCD poruke
    lcd_i2c_clear();
    lcd_i2c_set_cursor(0, 0);
    lcd_i2c_print("Upisi lozinku:");
    lcd_i2c_set_cursor(0, 1);
    lcd_i2c_print("                "); // očisti drugi red
}

int main(void)
{
    HAL_Init();          // Inicijalizacija HAL biblioteke i systick timera
    SystemClock_Config();// Postavke takta (HSI u ovom projektu)
    MX_GPIO_Init();      // Inicijalizacija GPIO pinova (iz CubeMX-a)
    MX_I2C1_Init();      // Inicijalizacija I2C1 (iz CubeMX-a)

    // Pokreni LCD (I2C adresa 0x27, 16x2 LCD)
    lcd_i2c_init(&hi2c1, 0x27, 16, 2);
    lcd_i2c_clear();
    lcd_i2c_set_cursor(0, 0);
    lcd_i2c_print("Upisi lozinku:"); // početna poruka
    lcd_i2c_set_cursor(0, 1);         // kursor na drugi red

    Keypad_Init();       // Inicijalizacija tipkovnice 4x4

    // Varijable za stanje sustava
    char input[PASSWORD_LEN + 1] = {0}; // buffer za unos (4 znaka + '\0')
    int idx = 0;                        // indeks unosa
    int unlocked = 0;                   // status otključano/zaključano
    int tocno = 0;                      // zadnji unos bio ispravan
    int fail_count = 0;                 // broj uzastopnih grešaka
    int locked = 0;                     // blokirano nakon 3 greške

    // Na početku ugasi signalizaciju
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

    // Glavna petlja
    while (1)
    {
        // Ako je pritisnuto hardversko tipkalo (PC13 → GND)
        if (HAL_GPIO_ReadPin(RESET_GPIO_Port, RESET_Pin) == GPIO_PIN_RESET) {
            HAL_Delay(80); // debounce
            reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked); // resetiraj sve
            while(HAL_GPIO_ReadPin(RESET_GPIO_Port, RESET_Pin) == GPIO_PIN_RESET); // čekaj otpuštanje
            HAL_Delay(80); // debounce
        }

        // Čitaj tipku s tipkovnice
        char key = Keypad_GetKey();
        if (key) {
            // Ako je sustav zaključan (3 greške)
            if (locked) {
                if (key == '*') { // '*' dozvoljava soft reset
                    reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
                }
                HAL_Delay(200); // debounce
                continue;       // preskoči logiku jer je zaključano
            }

            // '*' = soft reset unosa
            if (key == '*') {
                reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
            }
            // '#' = promjena lozinke
            else if (key == '#') {
                changeMode = 1;         // uđi u mod promjene
                idx = 0;                // novi unos
                input[0] = '\0';        // prazno
                lcd_i2c_clear();
                lcd_i2c_set_cursor(0, 0);
                lcd_i2c_print("Nova lozinka:"); // uputa korisniku
                lcd_i2c_set_cursor(0, 1);
            }
            // Inače unosimo lozinku znak po znak
            else if (idx < PASSWORD_LEN) {
                input[idx++] = key;         // spremi znak
                input[idx] = '\0';          // terminiraj string
                lcd_i2c_set_cursor(0, 1);   // pozicioniraj u drugi red
                lcd_i2c_print(input);       // prikaži unos
                for (int i = idx; i < 16; i++) lcd_i2c_print(" "); // očisti ostatak reda

                // Ako smo unijeli sva 4 znaka
                if (idx == PASSWORD_LEN) {
                    if (changeMode) {
                        // Promjena lozinke
                        strcpy(password, input);   // spremi novu lozinku
                        changeMode = 0;            // izađi iz moda
                        lcd_i2c_clear();
                        lcd_i2c_print("Lozinka promj."); // potvrda
                        buzzer_beep(1, 200);       // beep
                        HAL_Delay(800);            // kratka pauza
                        reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
                    } else {
                        // Provjera lozinke
                        if (strcmp(input, password) == 0) {
                            // TOČNO
                            lcd_i2c_clear();
                            lcd_i2c_print("Tocna lozinka!");
                            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // LED stalno ON
                            buzzer_beep(1, 200);                                     // beep
                            tocno = 1;      // označi da je točan unos
                            unlocked = 1;   // otključano
                            fail_count = 0; // reset grešaka
                        } else {
                            // POGREŠNO
                            fail_count++;               // broj grešaka++
                            lcd_i2c_clear();
                            lcd_i2c_print("Pogresna lozinka!");
                            led_blink(2, 200);          // LED blink dvaput
                            buzzer_beep(2, 120);        // buzzer beep dvaput
                            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // LED off
                            unlocked = 0;
                            tocno = 0;
                            idx = 0;         // reset unosa
                            input[0] = '\0';

                            if (fail_count >= 3) {
                                // Zaključaj nakon 3 greške
                                locked = 1;
                                lcd_i2c_clear();
                                lcd_i2c_set_cursor(0, 0);
                                lcd_i2c_print("Zakljucano!");
                                lcd_i2c_set_cursor(0, 1);
                                lcd_i2c_print("Reset na * ili tipk.");
                            } else {
                                // Inače vrati na početni ekran
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

        // Ako je lozinka točno unesena, LED stalno svijetli
        if (tocno) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        }
    }
}

// --- Konfiguracija sistemskog takta (HSI, bez PLL-a) ---
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();                                      // omogući PWR clock
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);   // naponsko skaliranje

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;       // koristimo HSI oscilator
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;                         // HSI uključen
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;                   // bez PLL-a
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();                                               // ako dođe do greške
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2; // konfiguriramo sve sabirnice
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;           // SYSCLK = HSI
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;               // AHB = HSI/1
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                // APB1 = HCLK/1
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;                // APB2 = HCLK/1

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();                                               // ako nešto ne štima
  }
}

// --- Funkcija za greške ---
void Error_Handler(void)
{
  __disable_irq(); // Onemogući prekide
  while (1) {      // Ostani ovdje (fail-safe loop)
  }
}
