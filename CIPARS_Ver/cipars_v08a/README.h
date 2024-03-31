/*  18.03.2024 - // https://codeload.github.com/UA6EM/Arduino-MCP4131/zip/refs/heads/mpgsp
    17.03.2024 - // https://codeload.github.com/UA6EM/MCP4151/zip/refs/heads/mpgsp

    16.03.2024 - регулировка MCP4131 продублирована на энкодер и кнопки
    07.03.2024 - проба регулировки усиления на MCP4131
    25.02.2024 - Версия CIPARS

    06.05.2022
    - Переработал программу для 2-строчного экрана

   11.06.2022
    - Во время работы отключил возможность крутить время
    - В меня при работе изменил Таймре на Т, добавил знак V
    - Добавил всем пинам имя
    - Определил пины для потенциометра ...
    - Добавил управление потенциометром с помощью энкодера

   03.07.2022
    - перенес инициализацию потенциометра в начало setup
*/
/*
#define SECONDS(x) ((x)*1000UL)
#define MINUTES(x) (SECONDS(x) * 60UL)
#define HOURS(x) (MINUTES(x) * 60UL)
#define DAYS(x) (HOURS(x) * 24UL)
#define WEEKS(x) (DAYS(x) * 7UL)
unsigned long interval = MINUTES(1);
unsigned long oneMinute = MINUTES(1);
unsigned long timers = MINUTES(5);  // время таймера 15, 30, 45 или 60 минут
*/

/*
#include <LiquidCrystal_I2C_Menu.h>       // https://github.com/VladimirTsibrov/LiquidCrystal_I2C_Menu
LiquidCrystal_I2C_Menu  lcd(0x3F, 16, 2); // https://tsibrov.blogspot.com/2020/09/LiquidCrystal-I2C-Menu.html
*/

/*
 * A0 -
 * A1 -
 * A2 -
 * A3 -
 * A4 - LCD1602
 * A5 - LCD1602
 * A6 -
 * A7 - SENS_IMPLOSION
 * D0 - RX
 * D1 - TX
 * D2 - AD9833 SW_MOSI
 * D3 - AD9833 SW_SCLK
 * D4 - AD9833 SW_CS (FSYNC)
 * D5 - LT1206_SHUTDOWN
 * D6 - ENC_DT
 * D7 - ENC_CLK
 * D8 - ENC_SW
 * D9 - BUZZER
 * D10 - MCP4151 CS
 * D11 - MCP4151 MOSI
 * D12 -
 * D13 - MCP4151 SCLK
 * 
 * D14 -
 * D15 -
 * D16 -
 * D17 -
 * D18 -
 * D19 -
 * /
 */
