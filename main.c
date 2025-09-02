#include "main.h"          // Osnovne definicije za projekt (HAL, tipovi, prototipovi)
#include "i2c.h"           // Inicijalizacija i rukovanje I2C periferiom
#include "gpio.h"          // Inicijalizacija i rukovanje GPIO pinovima
#include "lcd_i2c.h"       // Naša I2C LCD biblioteka (funkcije: init, clear, print, cursor)
#include "keypad.h"        // Naša tipkovnica 4x4 (Keypad_Init, Keypad_GetKey)
#include <string.h>        // strcmp, strcpy i dr. rad sa stringovima

#define PASSWORD_LEN  4     // Duljina lozinke (fiksno 4 znaka)

char password[PASSWORD_LEN + 1] = "1234";  // Trenutna lozinka 
int changeMode = 0;                        // Zastavica: 1 = u tijeku promjena lozinke

// Pinovi za LED i buzzer 
#define LED_GPIO_Port    GPIOA
#define LED_Pin          GPIO_PIN_11
#define BUZZER_GPIO_Port GPIOA
#define BUZZER_Pin       GPIO_PIN_12

// Tipkalo za reset (u CubeMX: PC13 kao Input s Pull-Up; tipkalo na GND)
#define RESET_GPIO_Port  GPIOC
#define RESET_Pin        GPIO_PIN_13

extern I2C_HandleTypeDef hi2c1;            // I2C1 handle (definiran u i2c.c od CubeMX-a)

// Kratko treptanje LED-ice n puta, s pauzom delay_ms između promjena stanja
void led_blink(int n, int delay_ms) {
    for (int i = 0; i < n; i++) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   // LED ON
        HAL_Delay(delay_ms);                                       // čekanje
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // LED OFF
        HAL_Delay(delay_ms);                                       // čekanje
    }
}

// Kratki "beep" buzzera n puta; svaki beep traje delay_ms, pauza 100 ms
void buzzer_beep(int n, int delay_ms) {
    for (int i = 0; i < n; i++) {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET); // Buzzer ON
        HAL_Delay(delay_ms);                                           // trajanje tona
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);// Buzzer OFF
        HAL_Delay(100);                                                // kratka pauza
    }
}

// Jedinstveno mjesto za "reset" stanja sustava i ekrana
// input      -> buffer u koji unosimo znakove
// idx        -> indeks sljedeće pozicije u inputu
// unlocked   -> je li brava "otključana"
// tocno      -> zadnji unos bio točan (služi da LED ostane stalno upaljena)
// fail_count -> broj uzastopnih pogrešnih pokušaja
// locked     -> zaključano nakon 3 greške; dozvoljen samo reset
void reset_unlock(char *input, int *idx, int *unlocked, int *tocno, int *fail_count, int *locked) {
    *idx = 0;                     // vrati unos na početak
    input[0] = '\0';              // prazni string
    *unlocked = 0;                // nije otključano
    *tocno = 0;                   // zadnji unos nije posebno označen kao točan
    *fail_count = 0;              // reset broja grešaka
    *locked = 0;                  // nije zaključano
    changeMode = 0;               // izađi iz moda promjene lozinke ako je bio aktivan

    // Utišaj signalizaciju
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

    // Resetiraj LCD i prikaži početnu poruku
    lcd_i2c_clear();
    lcd_i2c_set_cursor(0, 0);
    lcd_i2c_print("Upisi lozinku:");
    lcd_i2c_set_cursor(0, 1);
    lcd_i2c_print("                "); // očisti drugi red
}

int main(void)
{
    HAL_Init();          // Inicijalizacija HAL biblioteke i sistemskog ticka
    SystemClock_Config();// Postavke takta (HSI u ovom projektu)
    MX_GPIO_Init();      // Inicijalizacija svih GPIO-a (iz CubeMX-a)
    MX_I2C1_Init();      // Inicijalizacija I2C1 (iz CubeMX-a)

    // Pokretanje LCD-a (I2C adresa 0x27, 16x2)
    lcd_i2c_init(&hi2c1, 0x27, 16, 2);
    lcd_i2c_clear();
    lcd_i2c_set_cursor(0, 0);
    lcd_i2c_print("Upisi lozinku:"); // početna poruka
    lcd_i2c_set_cursor(0, 1);         // pozicioniraj na drugi red (prikaz unosa)

    Keypad_Init();       // Inicijalizacija tipkovnice (naše funkcije + CubeMX GPIO)

    char input[PASSWORD_LEN + 1] = {0}; // buffer za unos (4 znaka + '\0')
    int idx = 0;                        // trenutni indeks unosa
    int unlocked = 0;                   // je li otključano
    int tocno = 0;                      // zadnji unos bio točan
    int fail_count = 0;                 // uzastopne greške
    int locked = 0;                     // zaključano nakon 3 pogreške

    // Na početku ugasi signalizaciju
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

    while (1)
    {
        // Ako je pritisnuto "hardversko" tipkalo na PC13 (pull-up -> pritisak = 0)
        if (HAL_GPIO_ReadPin(RESET_GPIO_Port, RESET_Pin) == GPIO_PIN_RESET) {
            HAL_Delay(80); // debounce (filtriranje bounca)
            reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked); // vrati sve na početno
            while(HAL_GPIO_ReadPin(RESET_GPIO_Port, RESET_Pin) == GPIO_PIN_RESET); // pričekaj otpuštanje
            HAL_Delay(80); // još malo debounce
        }

        // Čitaj pritisnutu tipku s tipkovnice (0 ako ništa nije pritisnuto)
        char key = Keypad_GetKey();
        if (key) {
            // Ako je zaključano (3 greške), jedino '*' ili tipkalo mogu resetirati
            if (locked) {
                if (key == '*') {
                    reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
                }
                HAL_Delay(200); // debounce tipke
                continue;       // preskoči ostatak logike dok je zaključano
            }

            // '*' uvijek radi soft reset unosa i stanja (ne i promjenu lozinke)
            if (key == '*') {
                reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
            }
            // '#' ulazi u način promjene lozinke; sljedeća 4 unosa postaju nova lozinka
            else if (key == '#') {
                changeMode = 1;         // postavi mod promjene
                idx = 0;                // započni novi unos
                input[0] = '\0';        // prazno
                lcd_i2c_clear();
                lcd_i2c_set_cursor(0, 0);
                lcd_i2c_print("Nova lozinka:"); // uputa na ekranu
                lcd_i2c_set_cursor(0, 1);       // unos prikazujemo u drugom redu
            }
            // Inače, ako nismo popunili 4 znaka, dodaj pritisnutu tipku u buffer
            else if (idx < PASSWORD_LEN) {
                input[idx++] = key;         // spremi znak
                input[idx] = '\0';          // terminiraj string
                lcd_i2c_set_cursor(0, 1);   // idi na drugi red
                lcd_i2c_print(input);       // prikaži do sada uneseno
                for (int i = idx; i < 16; i++) lcd_i2c_print(" "); // očisti ostatak reda

                // Ako smo upravo unijeli 4 znaka, vrijeme je za akciju:
                if (idx == PASSWORD_LEN) {
                    if (changeMode) {
                        // U modu promjene lozinke: kopiraj novu lozinku i izađi iz moda
                        strcpy(password, input);   // spremi novu lozinku
                        changeMode = 0;            // gotovo s promjenom
                        lcd_i2c_clear();
                        lcd_i2c_print("Lozinka promj."); // poruka o uspjehu
                        buzzer_beep(1, 200);       // kratki beep
                        HAL_Delay(800);            // kratko zadrži poruku
                        // Vrati sustav na početno
                        reset_unlock(input, &idx, &unlocked, &tocno, &fail_count, &locked);
                    } else {
                        // Nismo u promjeni lozinke -> provjera unosa
                        if (strcmp(input, password) == 0) {
                            // TOČNO
                            lcd_i2c_clear();
                            lcd_i2c_print("Tocna lozinka!");
                            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // LED stalno ON
                            buzzer_beep(1, 200);                                     // jedan beep
                            tocno = 1;      // označi da je točno (da LED ostane upaljena)
                            unlocked = 1;   // "otključano"
                            fail_count = 0; // resetiraj broj grešaka
                        } else {
                            // POGREŠNO
                            fail_count++;               // brojimo uzastopne greške
                            lcd_i2c_clear();
                            lcd_i2c_print("Pogresna lozinka!");
                            led_blink(2, 200);          // LED trepne dvaput
                            buzzer_beep(2, 120);        // buzzer dva kratka beepa
                            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // LED ugasi
                            unlocked = 0;
                            tocno = 0;
                            idx = 0;         // isprazni unos
                            input[0] = '\0';

                            if (fail_count >= 3) {
                                // Nakon 3 greške: blokiraj daljnje unose
                                locked = 1;
                                lcd_i2c_clear();
                                lcd_i2c_set_cursor(0, 0);
                                lcd_i2c_print("Zakljucano!");
                                lcd_i2c_set_cursor(0, 1);
                                lcd_i2c_print("Reset na * ili tipk."); // uputa korisniku
                            } else {
                                // Inače samo vrati na početni ekran
                                lcd_i2c_set_cursor(0, 0);
                                lcd_i2c_print("Upisi lozinku:");
                                lcd_i2c_set_cursor(0, 1);
                                lcd_i2c_print("                ");
                            }
                        }
                    }
                }
            }
            HAL_Delay(200); // debounce svake tipke
        }

        // Ako je zadnja lozinka bila točna, LED neka i dalje gori
        if (tocno) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        }
    }
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();                                      // omogući PWR clock
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);   // naponsko skaliranje

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;       // koristimo interni HSI oscilator
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;                         // HSI uključen
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;                   // bez PLL u ovoj postavci
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();                                               // ako nešto krene po zlu
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2; // koje sabirnice podešavamo
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;           // SYSCLK = HSI
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;               // AHB bez djeljenja
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                // APB1 bez djeljenja
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;                // APB2 bez djeljenja

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();                                               // provjera latencije i postavki
  }
}

void Error_Handler(void)
{
  __disable_irq(); // onemogući prekide
  while (1) {      // ostani ovdje (fail-safe)
  }
}

