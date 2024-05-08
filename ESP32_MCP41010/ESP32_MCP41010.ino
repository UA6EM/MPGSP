// Генератор для катушки Мишина на основе DDS AD9833 для ESP32 экран LCD
// Partition Scheme: NO OTA (2MB APP, 2MB SPIFFS)
// При компиляции, в настройках IDE оключить все уведомления
// иначе вылетит по ошибке на sqlite3
// Важно!!! Измените настройки на Вашу WIFI сеть в конфигурационном файле
// Для библиотеки TFT_eSPI приведен конфигурационный файл  для ILI9341

//                    Использование GITHUB (если планируете участвовать в написании кода)
// 1. Клонируйте проект: git clone https://github.com/UA6EM/MPGSP
// 2. Исключите конфигурационный файл из индекса:
//    git update-index --assume-unchanged ESP32_MCP41010/config.h
//   (для отмены git update-index --no-assume-unchanged your_file)
// 3. Исправьте конфигурацию в соответствии с вашей сетью
//    Изменения в этом файле на локальном компьютере теперь
//    не попадут на GITHUB

/*
    Версия:
    08.05.2024 - Добавил возможности библиотеки GyverHUB ( управление FS)
    07.05.2024 - Добавил возможность работы с файловой системой LittleFS
    24.04.2024 - в работе версия для TFT дисплея
    04.04.2024 - проверена работа экрана LCD

    используемые библиотеки:
    Ai_Esp32_Rotary_Encoder-1.6.0                - https://www.arduino.cc/reference/en/libraries/ai-esp32-rotary-encoder/
    LiquidCrystal_I2C-master версии 1.1.4        - https://github.com/UA6EM/LiquidCrystal_I2C
    LCD_1602_RUS-master версии 1.0.5             - https://github.com/UA6EM/LCD_1602_RUS
    Ticker версии 2.0.                           - https://www.arduino.cc/reference/en/libraries/ticker/
    MCP4xxxx-ua6em версии 0.2                    - https://github.com/UA6EM/MCP4xxxx
    AD9833-mpgsp версии 0.4.0                    - https://github.com/UA6EM/AD9833/tree/mpgsp
                                                 - https://github.com/madhephaestus/ESP32Encoder
    esp32_arduino_sqlite3_lib-master версии 2.4  - https://github.com/siara-cc/esp32_arduino_sqlite3_lib
    Wire версии 2.0.0
    SPIFFS версии 2.0.0
    FS версии 2.0.0
    SPI версии 2.0.0
    WiFi версии 2.0.0                                  -
*/
#include <Arduino.h>
#define GH_INCLUDE_PORTAL
#define GH_FILE_PORTAL
#include <GyverHub.h>
GyverHub hub;

// Определения
#define WIFI                             // Используем модуль вайфая
#define DEBUG                            // Замаркировать если не нужны тесты
//#define LCD_RUS                          // Замаркировать, если скетч для пользователя CIPARS
#define SECONDS(x) ((x)*1000UL)
#define MINUTES(x) (SECONDS(x) * 60UL)
#define HOURS(x) (MINUTES(x) * 60UL)
#define DAYS(x) (HOURS(x) * 24UL)
#define WEEKS(x) (DAYS(x) * 7UL)

#define ON_OFF_CASCADE_PIN 32  // Для выключения выходного каскада
#define PIN_ZUM 33
#define CORRECT_PIN A3         // Пин для внешней корректировки частоты.

//ROTARY ENCODER
#define ROTARY_ENCODER_A_PIN 27 // Rotaty Encoder A  // orig16
#define ROTARY_ENCODER_B_PIN 26 // Rotaty Encoder B  // orig17
#define ROTARY_ENCODER_BUTTON_PIN 35
#define PIN_ENC_BUTTON ROTARY_ENCODER_BUTTON_PIN     // отдельная кнопка, для пробы
#define  MCP41010MOD            // библиотека с разрешением 255 единиц (аналог MCP4151)

// SD нужную включить, по умолчанию файловая система в SPIFFS
//#define SD_CARD
//#define SD_CARD_MMC

#if (defined(ESP32))
#ifdef WIFI
#include <WiFi.h>
//#include <HTTPClient.h>
#include <WiFiClient.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#include <SPI.h>
#include <FS.h>
#include "SPIFFS.h"
#include "LittleFS.h"
#include "SD_MMC.h"
#include "SD.h"
#include "config.h"
#include "driver/pcnt.h"
#include "Arduino.h"
#include "si5351.h"
#include <Wire.h>
Si5351 si5351;
boolean si5351_found = false;

#include <Ticker.h>
Ticker my_encoder;
float encPeriod = 0.05;
#endif

#define PIN_RELE 25

//LCD1602_I2C OR LCD2004_I2C AND INA219
#define I2C_SDA     21    // LCD1602 SDA
#define I2C_SCK     22    // LCD1602 SCK

//AD9833
#define AD9833_MISO 19    //12 //19
#define AD9833_MOSI 23    //13 //23
#define AD9833_SCK  18    //14 //18
#define AD9833_CS   15

//SD!!! ONLY
#define SD_MISO 19
#define SD_MOSI 23
#define SD_SCK  18
#define SD_CS    5

//MCP41010
#define  MCP41x1_MISO  19 //19 //12 // Define MISO pin for MCP4131 or MCP41010
#define  MCP41x1_MOSI  23 //23 //13 // Define MOSI pin for MCP4131 or MCP41010
#define  MCP41x1_SCK   18 //18 //14 // Define SCK pin for MCP4131 or MCP41010

#define  MCP41x1_CS    16  // Define chipselect pin for MCP41010 (CS for Volume)
#define  MCP41x1_ALC   17 // Define chipselect pin for MCP41010 (CS for ALC)

#define zFreq 2           // Делитель интервала - секунда/2

// Глобальные переменные
unsigned long interval = MINUTES(1);
unsigned long oneMinute = MINUTES(1);
unsigned long timers = MINUTES(5);  // время таймера 15, 30, 45 или 60 минут
unsigned long memTimers = 0;        //здесь будем хранить установленное время таймера
unsigned long oldmemTimers = 0;
bool isWorkStarted = 0;  // флаг запуска таймера

unsigned long timMillis = 0;
unsigned long oldMillis = 0;
unsigned long mill;  // переменная под millis()
unsigned long prevCorrectTime = 0;
unsigned long prevReadAnalogTime = 0;  // для отсчета 10 секунд между подстройкой частоты
unsigned long prevUpdateDataIna = 0;   // для перерыва между обновлениями данных ina
unsigned int  wiperValue;              // variable to hold wipervalue for MCP4131 or MCP4151
unsigned int Data_ina219 = 0;
long FREQ_MIN = 200000;                // 200kHz
long FREQ_MAX = 500000;                // 500kHz
long ifreq = FREQ_MIN;
long freq = FREQ_MIN;
long freqDisp;                         // Частота выводимая на дисплей
const unsigned long freqSPI = 250000;  // Частота только для HW SPI AD9833
// UNO SW SPI = 250kHz
const unsigned long availableTimers[] = { oneMinute * 15, oneMinute * 30, oneMinute * 45, oneMinute * 60 };
const byte maxTimers = 4;
int timerPosition = 0;
volatile int newEncoderPos;            // Новая позиция энкодера
static int currentEncoderPos = 0;      // Текущая позиция энкодера
volatile  int d_resis = 255;
volatile int alc_resis;                // Установка значения регулировки ALC (MCP41010)
bool SbLong = false;
bool SbClic = false;

int16_t RE_Count = 0;

#include <Wire.h>
#include <SPI.h>

#include <MCP4xxxx.h>  // https://github.com/UA6EM/MCP4xxxx
MCP4xxxx Potentiometer(MCP41x1_CS, MCP41x1_MOSI, MCP41x1_SCK, 250000UL, SPI_MODE0);
MCP4xxxx Alc(MCP41x1_ALC, MCP41x1_MOSI, MCP41x1_SCK, 250000UL, SPI_MODE0);
//MCP4xxxx Potentiometer(MCP41x1_CS);
// Библиотека требует инициализации объекта в setup - Alc.begin(); Potentiometer.begin();


#include "INA219.h"
INA219 ina219;

// по умолчанию 50% потенциометра
int currentPotenciometrPercent = 127;


//--------------- Create an AD9833 object ----------------
/*
  #include <AD9833.h>  // Пробуем новую по ссылкам в README закладке
  //AD9833 AD(10, 11, 13);     // SW SPI over the HW SPI pins (UNO);
  //AD9833 Ad9833(AD9833_CS);  // HW SPI Defaults to 25MHz internal reference frequency
  AD9833 Ad9833(AD9833_CS, AD9833_MOSI, AD9833_SCK); // SW SPI speed 250kHz
*/
#include <MD_AD9833.h>
//MD_AD9833  Ad9833(PIN_FSYNC);  // Hardware SPI
MD_AD9833  Ad9833(AD9833_MOSI, AD9833_SCK, AD9833_CS); // Arbitrary SPI pins


/******* Простой энкодер *******/

// Дисплей TFT ILI-9341
#define TFT_GREY 0x5AEB
#include <TFT_eSPI.h>            // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library


//    *** Используемые подпрограммы выносим сюда ***   //
/*--------------------------------------------------------------------------
        Timer ISR
  ---------------------------------------------------------------------------*/
hw_timer_t * timer = NULL;
void IRAM_ATTR onTimer() {}


// переменные для часиков
float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = 120, osy = 120, omx = 120, omy = 120, ohx = 120, ohy = 120; // Saved H, M, S x & y coords
uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;
uint32_t targetTime = 0;                    // for next 1 second timeout
static uint8_t conv2d(const char* p);       // Forward declaration needed for IDE 1.6.x
//uint8_t hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time
uint8_t hh = 12, mm = 0, ss = 0; // Get H, M, S from compile time
static int old_mm = 0;

bool initial = 1;
bool isduble = false;


/* ********* Ч А С И К И ********* */
void testTFT(int times) {
  int my_times = times / 1000;
  if (!isduble) {

    isduble = true;
    tft.init();
    tft.setRotation(0);
    //tft.fillScreen(TFT_BLACK);
    //tft.fillScreen(TFT_RED);
    //tft.fillScreen(TFT_GREEN);
    //tft.fillScreen(TFT_BLUE);
    //tft.fillScreen(TFT_BLACK);
    tft.fillScreen(TFT_GREY);
    tft.setTextColor(TFT_WHITE, TFT_GREY);  // Adding a background colour erases previous text automatically
    // Draw clock face
    tft.fillCircle(120, 120, 118, TFT_GREEN);
    tft.fillCircle(120, 120, 110, TFT_BLACK);
    // Draw 12 lines
    for (int i = 0; i < 360; i += 30) {
      sx = cos((i - 90) * 0.0174532925);
      sy = sin((i - 90) * 0.0174532925);
      x0 = sx * 114 + 120;
      yy0 = sy * 114 + 120;
      x1 = sx * 100 + 120;
      yy1 = sy * 100 + 120;
      tft.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
    }
    // Draw 60 dots
    for (int i = 0; i < 360; i += 6) {
      sx = cos((i - 90) * 0.0174532925);
      sy = sin((i - 90) * 0.0174532925);
      x0 = sx * 102 + 120;
      yy0 = sy * 102 + 120;
      // Draw minute markers
      tft.drawPixel(x0, yy0, TFT_WHITE);
      // Draw main quadrant dots
      if (i == 0 || i == 180) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
      if (i == 90 || i == 270) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
    }
    tft.fillCircle(120, 121, 3, TFT_WHITE);
    // Draw text at position 120,260 using fonts 4
    // Only font numbers 2,4,6,7 are valid. Font 6 only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : . - a p m
    // Font 7 is a 7 segment font and only contains characters [space] 0 1 2 3 4 5 6 7 8 9 : .
    //yield();
    //tft.drawCentreString("Time flies", 120, 275, 4);
  }

  // Draw expositions dots
  for (int i = 0; i < 360; i ++) {
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * 102 + 120;
    yy0 = sy * 102 + 120;
    // Draw minute markers
    // tft.drawPixel(x0, yy0, TFT_WHITE);
    // Draw main quadrant dots
    //   if (i == 0 || i == 180) tft.fillCircle(x0, yy0, 2, TFT_WHITE);
    if (old_mm > 0) {
      if (i == old_mm) {
        tft.fillCircle(x0, yy0, 7, TFT_BLUE);
        tft.fillCircle(x0, yy0, 3, TFT_YELLOW);
      }
    }
    if (i == (mm * 6 + (times / 1000 / 10))) tft.fillCircle(x0, yy0, 7, TFT_ORANGE);
    if (i == (mm * 6 + (times / 1000 / 10))) tft.fillCircle(x0, yy0, 3, TFT_RED);
  }
  old_mm = (mm * 6 + (times / 1000 / 10));

  targetTime = millis() + 1000;
  unsigned long ttt = millis();

  while (millis() - ttt < times) {
    if (targetTime < millis()) {
      targetTime += 1000;

      yield();
      tft.drawCentreString("                     ", 120, 250, 4);
      if (my_times <= 100) {
        tft.drawCentreString("                     ", 120, 250, 4);
        tft.drawNumber(--my_times, 110, 250, 4);
        yield();
        tft.drawCentreString("                     ", 120, 275, 2);
        yield();
        tft.drawCentreString("MPGSP is ON", 120, 275, 2);
      } else {
        tft.drawCentreString("                     ", 120, 250, 4);
        tft.drawNumber(--my_times, 100, 250, 4);
        yield();
        tft.drawCentreString("                     ", 120, 275, 2);
        yield();
        tft.drawCentreString("MPGSP is ON", 120, 275, 2);
      }

      ss++;              // Advance second
      if (ss == 60) {
        ss = 0;
        mm++;            // Advance minute
        if (mm > 59) {
          mm = 0;
          hh++;          // Advance hour
          if (hh > 23) {
            hh = 0;
          }
        }
      }

      // Pre-compute hand degrees, x & y coords for a fast screen update
      sdeg = ss * 6;                // 0-59 -> 0-354
      mdeg = mm * 6 + sdeg * 0.01666667; // 0-59 -> 0-360 - includes seconds
      hdeg = hh * 30 + mdeg * 0.0833333; // 0-11 -> 0-360 - includes minutes and seconds
      hx = cos((hdeg - 90) * 0.0174532925);
      hy = sin((hdeg - 90) * 0.0174532925);
      mx = cos((mdeg - 90) * 0.0174532925);
      my = sin((mdeg - 90) * 0.0174532925);
      sx = cos((sdeg - 90) * 0.0174532925);
      sy = sin((sdeg - 90) * 0.0174532925);

      if (ss == 0 || initial) {
        initial = 0;
        // Erase hour and minute hand positions every minute
        tft.drawLine(ohx, ohy, 120, 121, TFT_BLACK);
        ohx = hx * 62 + 121;
        ohy = hy * 62 + 121;
        tft.drawLine(omx, omy, 120, 121, TFT_BLACK);
        omx = mx * 84 + 120;
        omy = my * 84 + 121;
      }

      // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
      tft.drawLine(osx, osy, 120, 121, TFT_BLACK);
      osx = sx * 90 + 121;
      osy = sy * 90 + 121;
      tft.drawLine(osx, osy, 120, 121, TFT_RED);
      tft.drawLine(ohx, ohy, 120, 121, TFT_WHITE);
      tft.drawLine(omx, omy, 120, 121, TFT_WHITE);
      tft.drawLine(osx, osy, 120, 121, TFT_RED);

      tft.fillCircle(120, 121, 3, TFT_RED);
    }
  }
  yield();
  tft.drawCentreString("                     ", 120, 250, 4);
  yield();
  tft.drawCentreString("                     ", 120, 275, 2);
}

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}
// ******************* КОНЕЦ БЛОКА ФУНКЦИЙ ПО РАБОТЕ С ЧАСИКАМИ ******************//


// ******************** БЛОК ФУНКЦИЙ ПО РАБОТЕ В БАЗОЙ ДАННЫХ ********************//
#define FORMAT_SPIFFS_IF_FAILED true
#define FORMAT_LITTLEFS_IF_FAILED true

int zepDbFreq[42];    // массив из таблицы частот
int zepDbExpo[42];    // массив из таблицы экспозиций
int zepDbPause[42];   // массив из таблицы пауз
int zepDbModeGen[42]; // массив из таблицы режима генератора
int zepDbModeSig[42]; // массив из таблицы вида формируемого сигнала

char *zErrMsg = 0;
char buffers[33];

int numerInTable = 1;

// Строки запроса к таблица (заготовка) к ней добавляется id
String queryFreq = "SELECT * FROM frequencys WHERE id = ";
String queryExpo = "SELECT * FROM expositions WHERE id = ";
String queryPause = "SELECT * FROM pauses WHERE id = ";
String queryGen = "SELECT * FROM modesgen WHERE id = ";
String querySig = "SELECT * FROM modessig WHERE id = ";

#ifdef SD_CARD
const char* DBName = "/sd/standard1.db";
#else
#ifdef SD_CARD_MMC
const char* DBName = "/sdcard/standard1.db";
#else
#ifdef LITTLEFS
const char* DBName = "/littlefs/standard1.db";
#else
const char* DBName = "/spiffs/standard1.db";
//const char* DBName = "/spiffs/zepper.db";
//const char* DBName = "/spiffs/std.db";
#endif
#endif
#endif


struct  {
  int ModeGen;   // 0 - GENERATOR 1 - ZEPPER
  int ModeSig;   // 0 - OFF, 1 - SINE, 2 - QUADRE1, 3 - QUADRE2, 4 - TRIANGLE
  int Freq;      // Частота сигнала (long, float)
  int Exposite;  // Время экспозиции в секундах
  int Pause;     // Пауза после отработки времени экспозиции в секундах
} Cicle;

// Универсальный callback
// Вызывается столько раз, сколько строк вернул запрос. Т.е. один вызов на строку!
// Если вернуть не 0, то обработка запроса прекратится немедленно
// Параметры:
//    tagregArray - принимающий массив. НЕ ПРОВЕРЯЕТСЯ, что его РАЗМЕР ДОСТАТОЧЕН
//    totalColumns - количество колонок
//    azColVals - массив размером totalColumns - значения колонок в текстовом виде (если значение NULL, то нулевой указатель)
//    azColNames - массив размером totalColumns - имена колонок
//
// ВНИМАНИЕ:   значение поля f1 помещается а НУЛЕВОЙ элемент принимающего массива,
//             f2 - в первый, f3 - во второй и т.п.!
//
static int callback(void* tagregArray, int totalColumns, char** azColVals, char** azColNames) {
  int* target = reinterpret_cast<int*>(tagregArray);
  for (int i = 0; i < totalColumns; i++) {
    const char* colName = azColNames[i];
    const char nameFirstChar = colName[0];
    if (nameFirstChar == 'f') {
      const int index = atoi(colName + 1) - 1;
      target[index] = azColVals[i] ? atoi(azColVals[i]) : 0;
    }
  }
  return 0;
}

int db_open(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}

void printStruct() {
#ifdef DEBUG
  Serial.print("Cicle.ModeGen  = ");
  Serial.println(Cicle.ModeGen);

  Serial.print("Cicle.ModeSig  = ");
  Serial.println(Cicle.ModeSig);

  Serial.print("Cicle.Freq     =");
  Serial.println(Cicle.Freq);

  Serial.print("Cicle.Exposite = ");
  Serial.println(Cicle.Exposite);

  Serial.print("Cicle.Pause    = ");
  Serial.println(Cicle.Pause);
#endif
}
// ******************* КОНЕЦ БЛОКА ФУНКЦИЙ ПО РАБОТЕ С БАЗОЙ  ******************//

/*-----------------------------------------------------------------------------------------------
        Alternative Loop (core0)
  ------------------------------------------------------------------------------------------------*/
void task0(void* arg)
{
  while (1)
  {
    pcnt_get_counter_value(PCNT_UNIT_0, &RE_Count);
    static int count; //=RE_Count;
    if (count != RE_Count / 4) {
      count = RE_Count / 4;
      Serial.println(count);
      newEncoderPos = count;
    }
    //pcnt_counter_clear(PCNT_UNIT_0);
    //Serial.println(count);
    delay(1);
    hub.tick();
  }
}


/*** Обработчик кнопки энкодера ***/
//------Cl_Btn----------------------
enum { sbNONE = 0,
       sbClick,
       sbLong
     }; /*состояние не изменилось/клик/долгое нажатие*/
class Cl_Btn {
  protected:
    const byte pin;
    byte state;
    bool bounce = 0;
    bool btn = 1, oldBtn;
    unsigned long past;
    const uint32_t time = 500;
    bool flag = 0;
    uint32_t past_flag = 0;
  public:
    Cl_Btn(byte p)
      : pin(p) {}
    /*инициализация-вставить в setup()*/
    void init() {
      pinMode(pin, INPUT_PULLUP);
    }
    /*работа-вставить в loop()*/
    void run() {
      state = sbNONE;
      bool newBtn = digitalRead(pin);
      if (!bounce && newBtn != btn) {
        bounce = 1;
        past = mill;
      }
      if (bounce && mill - past >= 10) {
        bounce = 0;
        oldBtn = btn;
        btn = newBtn;
        if (!btn && oldBtn) {
          flag = 1;
          past_flag = mill;
        }
        if (!oldBtn && btn && flag && mill - past_flag < time) {
          flag = 0;
          state = sbClick;
        }
      }
      if (flag && mill - past_flag >= time) {
        flag = 0;
        state = sbLong;
      }
    }
    byte read() {
      return state;
    }
};
Cl_Btn Btn1(PIN_ENC_BUTTON);  //Экземпляр обработчика для кнопки энкодера

//
int getALC(long freq) {
  int alc = map(freq, 50000, 1000000, 0, 255);
  return alc;
}

void setALC(int setAlc) {
  Alc.writeValue(setAlc);
  delay(10);
}

void setAlcFreq(long freq) { // Установка по измеренному уровню сигнала
  int alc;
  if (freq >= 50000)alc = 0;
  if (freq >= 100000)alc = 14;
  if (freq >= 150000)alc = 27;
  if (freq >= 200000)alc = 40;
  if (freq >= 250000)alc = 53;
  if (freq >= 300000)alc = 66;
  if (freq >= 350000)alc = 80;
  if (freq >= 400000)alc = 93;
  if (freq >= 450000)alc = 106;
  if (freq >= 500000)alc = 120;
  if (freq >= 550000)alc = 133;
  if (freq >= 600000)alc = 146;
  if (freq >= 650000)alc = 160;
  if (freq >= 700000)alc = 173;
  if (freq >= 750000)alc = 186;
  if (freq >= 800000)alc = 200;
  if (freq >= 850000)alc = 213;
  if (freq >= 900000)alc = 226;
  if (freq >= 950000)alc = 240;
  if (freq >= 1000000)alc = 254;
  Alc.writeValue(alc);
  delay(10);
}

// функция выбора времени работы
void setTimer() {
  // если энкодер крутим по часовой
  if (newEncoderPos - currentEncoderPos > 0) {
    if (timerPosition == maxTimers - 1) {
      timerPosition = 0;
    } else {
      timerPosition += newEncoderPos - currentEncoderPos;
    }
  } else if (newEncoderPos - currentEncoderPos < 0) {
    // если энкодер крутим против часовой
    if (timerPosition == 0) {
      timerPosition = maxTimers - 1;
    } else {
      timerPosition -= abs(newEncoderPos - currentEncoderPos);
    }
  }
  memTimers = availableTimers[timerPosition];
}


void resetPotenciometer() {
  // Понижаем сопротивление до 0%:
  wiperValue = 1;
  Potentiometer.writeValue(wiperValue);  // Set MCP4151 to position 1
}

// Уровень percent - от 0 до 100% от максимума.
void setResistance(int percent) {
  // resetPotenciometer();
  // for (int i = 0; i < percent; i++) {
  wiperValue = percent;;
  // }
  Potentiometer.writeValue(wiperValue);  // Set MCP4151
}

void processPotenciometr() {
  // если энкодер крутим по часовой
  if (newEncoderPos - currentEncoderPos > 0) {
    if (currentPotenciometrPercent >= d_resis) {
      currentPotenciometrPercent = d_resis;
      wiperValue = d_resis;
    } else {
      currentPotenciometrPercent += newEncoderPos - currentEncoderPos; //1;
      wiperValue += newEncoderPos - currentEncoderPos; //1;
    }
  } else if (newEncoderPos - currentEncoderPos < 0) {
    // если энкодер крутим против часовой
    if (currentPotenciometrPercent <= 1) {
      currentPotenciometrPercent = 1;
      wiperValue = 1;
    } else {
      currentPotenciometrPercent -= abs(newEncoderPos - currentEncoderPos); //1;
      wiperValue -= abs(newEncoderPos - currentEncoderPos); //1;
    }
  }
  setResistance(currentPotenciometrPercent);
  Potentiometer.writeValue(wiperValue);  // Set MCP41010 to mid position
  Serial.print("wiperValue = ");
  Serial.println(wiperValue);
}


/********* Таймер обратного отсчёта экспозиции **********/
unsigned long setTimerLCD(unsigned long timlcd) {
  if (millis() - timMillis >= 1000) {
    timlcd = timlcd - 1000;
    timMillis += 1000;
  }
  if (timlcd == 0) {
    timlcd = oldmemTimers;
    isWorkStarted = 0;
    //lcd.setCursor(0, 1);
#ifdef LCD_RUS
    yield();
    tft.drawCentreString("СТОП", 120, 275, 2);   //Serial.println("    СТОП!     ");
#else
    yield();
    tft.drawCentreString("STOP", 120, 275, 2);   //Serial.println("    STOP!     ");
#endif
    digitalWrite(ON_OFF_CASCADE_PIN, LOW);
    si5351.output_enable(SI5351_CLK0,0);
    start_Buzzer();
    delay(3000);
    stop_Buzzer();
    // lcd.setCursor(0, 1);
    yield();
    tft.drawCentreString("            ", 120, 275, 2);  //Serial.println("              ");
  }
  return timlcd;
}

/*******************ПИЩАЛКА ********************/
void start_Buzzer() {
  digitalWrite(PIN_ZUM, HIGH);
}

void stop_Buzzer() {
  digitalWrite(PIN_ZUM, LOW);
}

// ******************* Обработка AD9833 ***********************
void /*long*/ readAnalogAndSetFreqInSetup() {
  int maxValue = 0;
  long freqWithMaxI = FREQ_MIN;
  long freqIncrease = 1000;                                   // 1kHz
  int iterations = (FREQ_MAX - FREQ_MIN) / freqIncrease - 1;  // (500000 - 200000) / 1000 - 1 = 199

  for (int j = 1; j <= iterations; j++) {
    // читаем значение аналогового входа
    int tempValue = analogRead(CORRECT_PIN);
    // если значение тока больше предыдущего, запоминаем это значение и текущую частоту
    if (tempValue > maxValue) {
      maxValue = tempValue;
      freqWithMaxI = freq;
    }
    // увеличиваем частоту для дальнейшего измерения тока
    freq = freq + freqIncrease;
    if (freq > FREQ_MAX) {
      freq = FREQ_MAX;
    }
    // подаём частоту на генератор
    //Ad9833.setFrequency((float)freq, AD9833_SINE);
    Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)freq);
    delay(20);
  }
  ifreq = freqWithMaxI;
  // подаём частоту на генератор
  //Ad9833.setFrequency((float)ifreq, AD9833_SINE);
  Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)ifreq);
  prevReadAnalogTime = millis();
}

/**** Подстройка частоты каждые 1-10 секунд относительно аналогового сигнала ***/
void readAnalogAndSetFreqInLoop() {
  unsigned long curr = millis();

  // если прошло N секунд с момента последней проверки
  if (curr - prevReadAnalogTime > 1000 * 5) {  //выбор времени изменения частоты.1-10 сек.
    long availableDiff = 5000;                 // 1kHz-10kHz разница частот
    long freqIncrease = 500;                   // 100Hz-1kHz шаг увеличения частоты при сканировании

    int iterations = (availableDiff * 2) / freqIncrease - 1;  // (10000 * 2) / 1000 - 1 = 19

    long minimalFreq = ifreq - availableDiff;
    if (minimalFreq < FREQ_MIN) {
      minimalFreq = FREQ_MIN;
    }
    // подаём на генератор минимальную частоту из диапазона +-10кГц
    //Ad9833.setFrequency((float)minimalFreq, AD9833_SINE);
    Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)minimalFreq);
    delay(20);

    int maxValue = 0;
    long freqWithMaxI = minimalFreq;
    freq = minimalFreq;

    for (int j = 1; j <= iterations; j++) {
      // читаем значение аналогового входа
      int tempValue = analogRead(CORRECT_PIN);
      // если значение тока больше предыдущего, запоминаем это значение и текущую частоту
      if (tempValue > maxValue) {
        maxValue = tempValue;
        freqWithMaxI = freq;
      }
      // увеличиваем частоту для дальнейшего измерения тока
      freq = freq + freqIncrease;
      if (freq > FREQ_MAX) {
        freq = FREQ_MAX;
      }
      // подаём частоту на генератор
      //Ad9833.setFrequency((float)freq, AD9833_SINE);
      Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)freq);
      delay(10);
    }
    ifreq = freqWithMaxI;
    freqDisp = ifreq;
    //Ad9833.setFrequency((float)ifreq, AD9833_SINE);
    Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)ifreq);
    prevReadAnalogTime = millis();
  }
  alc_resis = getALC(ifreq); // рассчитать значение усиления сигнала
  setALC( alc_resis);        // выставить усиление
}


// *** Вывод на дисплей ***
String s = "";
String s1 = "";

void tftDisplay() {
  // 1-я строка
  s = "";
  if (!isWorkStarted) {
#ifdef LCD_RUS
    s += "Время-"; //Serial.println("Время-");
#else
    s += "Times-"; //Serial.println("Times-");
#endif
    s += String(memTimers / 60000); //Serial.println(memTimers / 60000);
    if (memTimers / 60000 > 0) {
#ifdef LCD_RUS
      s += " мин. ";  //Serial.println(" мин. ");
#else
      s += " min. ";  //Serial.println(" min. ");
#endif
    } else {
#ifdef LCD_RUS
      s += "0 мин. "; // Serial.println("0 мин. ");
#else
      s += "0 min. "; //  Serial.println("0 min. ");
#endif
    }
  } else {
    s += "T-";
    //Serial.print("T-");
    if (memTimers > 60000) {
      // если больше минуты, то показываем минуты
      s += String(memTimers / 1000 / 60);
      //Serial.println(memTimers / 1000 / 60);
#ifdef LCD_RUS
      s += "мин."; // Serial.println("мин.");
#else
      s += "min."; // Serial.println("min.");
#endif
    } else {
      // если меньше минуты, то показываем секунды
      // Serial.println(memTimers / 1000);
      s += String(memTimers / 1000);
#ifdef LCD_RUS
      s += "сек."; //Serial.println("сек.");
#else
      s += "sek."; //Serial.println("sek.");
#endif
    }
    s += " U="; //Serial.println(" U=");
#ifdef MCP41010MOD  // можно замапить в реальный % выходного сигнала
    //itoa(numerInTable, buffers, 10);
    s += String(map(currentPotenciometrPercent, 1, 255, 1, 100)); //Serial.println(map(currentPotenciometrPercent, 1, 255, 1, 100));
#else
    s += String(map(currentPotenciometrPercent, 1, 127, 1, 100));  //Serial.println(map(currentPotenciometrPercent, 1, 127, 1, 100));
#endif
    s += "%  "; //Serial.println("%  ");
  }

  yield();
  tft.drawCentreString("                         ", 120, 275, 2);
  yield();
  tft.drawCentreString(s, 120, 275, 2);

  // 2 строка Частота
  s1 = "";
  s1 += " F="; //  Serial.println("F=");
  float freq_tic = freqDisp;
  float kHz = freq_tic / 1000;
 
  s1 += String(kHz);               //Serial.println(kHz, 0);
  s1 += " kHz  ";                  //(Serial.println("kHz");

  // 2 строка Ток
  s1 += "I = ";                     //Serial.println("I=");
  //lcd.setCursor(11, 1);
  s1 += String(Data_ina219 * 2);  //Serial.println(Data_ina219 * 2);
  s1 += " ma";                     //Serial.println("ma");
  yield();
  tft.drawCentreString("                          ", 120, 293, 2);
  yield();
  tft.drawCentreString(s1, 120, 293, 2);
}


int readSqlDB() {
#ifdef SD_CARD_MMC
  SPI.begin();
  SD_MMC.begin();
#else
#ifdef SD_CARD
  //SPI.begin(-1,-1,-1,SD_CS);
  SD.begin(SD_CS);
  //SPI.begin();
  //SD.begin();
#else
#ifdef LITTLEFS
if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
       Serial.println("Failed to mount file system");
       return 0;
   }
#else
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("Failed to mount file system");
    return 0;
  }
#endif
#endif
#endif

  sqlite3* db;
  sqlite3_initialize();
  int rc = sqlite3_open(DBName /*"/spiffs/zepper.db"*/, &db);
  if (rc) {
    printf("Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return (1);
  }

  // Принимающий массив передаётся чертвёртым параметром!
  // Формируем строки запросов к таблицам базы данных
  itoa(numerInTable, buffers, 10);
  queryFreq  += String(buffers);
  queryExpo  += String(buffers);
  queryPause += String(buffers);
  queryGen   += String(buffers);
  querySig   += String(buffers);

  Serial.println();
  Serial.print("миллис начала = ");
  unsigned long gomill = millis();
  Serial.println(gomill);
  Serial.println();

  //заполним массив частот
  rc = sqlite3_exec(db, queryFreq.c_str(), callback, zepDbFreq, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
    // ПЕЧАТЬ массива для проверки
#ifdef DEBUG
    for (int i = 0; i < sizeof(zepDbFreq) / sizeof(zepDbFreq[0]); i++) {
      printf("zepDbFreq[%d]=%d\n", i, zepDbFreq[i]);
    }
#endif
  }
  Serial.print(queryFreq);
  Serial.println("  - Обработан");

  //заполним массив экспозиций
  rc = sqlite3_exec(db, queryExpo.c_str(), callback, zepDbExpo, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
#ifdef DEBUG
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbExpo) / sizeof(zepDbExpo[0]); i++) {
      printf("zepDbExpo[%d]=%d\n", i, zepDbExpo[i]);
    }
#endif
  }
  Serial.print(queryExpo);
  Serial.println("  - Обработан");

  //заполним массив пауз
  rc = sqlite3_exec(db, queryPause.c_str(), callback, zepDbPause, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
#ifdef DEBUG
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbPause) / sizeof(zepDbPause[0]); i++) {
      printf("zepDbPause[%d]=%d\n", i, zepDbPause[i]);
    }
#endif
  }
  Serial.print(queryPause);
  Serial.println("  - Обработан");

  //заполним массив режимов генератора
  rc = sqlite3_exec(db, queryGen.c_str(), callback, zepDbModeGen, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
#ifdef DEBUG
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbModeGen) / sizeof(zepDbModeGen[0]); i++) {
      printf("zepDbModeGen[%d]=%d\n", i, zepDbModeGen[i]);
    }
#endif
  }
  Serial.print(queryGen);
  Serial.println("  - Обработан");

  //заполним массив режимов вида сигнала генератора
  rc = sqlite3_exec(db, querySig.c_str(), callback, zepDbModeSig, &zErrMsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  else {
#ifdef DEBUG
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbModeSig) / sizeof(zepDbModeSig[0]); i++) {
      printf("zepDbModeSig[%d]=%d\n", i, zepDbModeSig[i]);
    }
#endif
  }
  Serial.print(querySig);
  Serial.println("  - Обработан");

  Serial.println();
  Serial.print("миллис окончания = ");
  Serial.println(millis());
  Serial.print("Время обработки базы = ");
  Serial.print((float)(millis() - gomill) / 1000, 3);
  Serial.println(" сек");
  Serial.println();

  sqlite3_close(db);
  setStructure(0);
  printStruct();

#ifdef SD_CARD_MMC
  SD_MMC.end();
#else
#ifdef SD_CARD
  SD.end();
#else
#ifdef LITTLEFS
  LittleFS.end();
#else
  SPIFFS.end();
#endif
#endif
#endif

  return 0;
}    // ************** END SQLITE3 *************** //


// ************** TEST MCP41010 *************** //
void testMCP41010() {
  d_resis = 255;

  Serial.println("START Test MCP41010");
  for (int i = 0; i < d_resis; i++) {
    Potentiometer.writeValue(i);
    delay(100);
    Serial.print("MCP41010 = ");
    Serial.println(i);
  }
  for (int j = d_resis; j >= 1; --j) {
    Potentiometer.writeValue(j);
    delay(100);
    Serial.print("MCP41010 = ");
    Serial.println(j);
  }
  Serial.println("STOP Test MCP41010");
}// ************** END TEST MCP41010 *************** //



//************************** SETUP *************************/
void setup() {

  // Управляющие
  pinMode(ON_OFF_CASCADE_PIN, OUTPUT);
  pinMode(PIN_ZUM, OUTPUT);
  pinMode(PIN_RELE, OUTPUT);
  pinMode(CORRECT_PIN, INPUT);
  digitalWrite(PIN_ZUM, LOW);
  digitalWrite(PIN_RELE, LOW);
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
  si5351.output_enable(SI5351_CLK0,1);
  // Энкодер
  pinMode(ROTARY_ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ROTARY_ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(ROTARY_ENCODER_BUTTON_PIN, INPUT_PULLUP);

  // устройства SPI
  pinMode(AD9833_CS, OUTPUT);   // AD9833
  pinMode(MCP41x1_CS, OUTPUT);  // MCP41010 - 2
  pinMode(MCP41x1_ALC, OUTPUT); // MCP41010 - 1
  pinMode(SD_CS, OUTPUT);       // SD
  digitalWrite(AD9833_CS, HIGH);   // OFF
  digitalWrite(MCP41x1_CS, HIGH);  // OFF
  digitalWrite(MCP41x1_ALC, HIGH); // OFF
  digitalWrite(SD_CS, HIGH);       // OFF

  Serial.begin(115200);
  Serial.println("START");
  delay(100);

  // Читаем базу
  readSqlDB();                        // Чтение базы в массивы с трёх возможных накопителей

  Btn1.init();                        // Инициализируем кнопку

  ina219.begin(0x40);                 // (44) i2c address 64=0x40 68=0х44 исправлять и в ina219.h одновременно
  ina219.configure(0, 2, 12, 12, 7);  // 16S -8.51ms
  ina219.calibrate(0.100, 0.32, 16, 3.2);

  //SPI.begin();    // Для библиотеки MCP4xxxx обязательно было, теперь нельзя!!!
  Potentiometer.begin();
  Alc.begin();

  // сбрасываем потенциометр в 0%
  resetPotenciometer();
  // после сброса устанавливаем значение по умолчанию
  setResistance(currentPotenciometrPercent);

  // ждем после настройки потенциометра
  delay(100);


  // Инициализируем AD9833
  Ad9833.begin();          //
  Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)FREQ_MIN);
  Ad9833.setMode(MD_AD9833::MODE_SINE);

  Serial.print("freq=");
  Serial.print(FREQ_MIN);
  Serial.println(" Hz");

  // SI5351 Инициализировать и вывести частоту
  Wire.begin();
  si5351_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);//если нашёлся si5351 поднимается флажок
  // if (si5351_found) { si5351.set_freq(16E6*SI5351_FREQ_MULT, SI5351_CLK1);// фиксированная частота с выхода 1
  // si5351.set_freq(20E6*SI5351_FREQ_MULT, SI5351_CLK2); }//фиксированная частота с выхода 2
  setSI5351Freq(FREQ_MIN); // выход на CLK0 (0) пин 0

  Data_ina219 = ina219.shuntCurrent() * 1000;

#ifndef TFT_ERR     // отклюжчение вывода на дисплей в связи с ошибками в подпрограмме
  tftDisplay();
#endif

  delay(100);

  memTimers = availableTimers[0];  // выставляем 15 минут по умолчанию
#ifdef DEBUG
  // testMCP41010();
#endif

  wiperValue = d_resis / 2;        // половинная мощность
  //currentEncoderPos = wiperValue;
  Potentiometer.writeValue(wiperValue);  // Set MCP4131 or MCP4151 to mid position

#ifdef DEBUG
  testTFT(10000);
  //old_mm = 1;
#endif

  // Энкодер
  //--------- create tasks on core0 --------------------------------
  xTaskCreatePinnedToCore(task0, "Task0", 4096, NULL, 1, NULL, 0);

  /*
    //--------- Set up Interrupt Timer -------------------------------
    timer = timerBegin(0, 80, true); //use Timer0, div80 for 1us clock
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 10000, true); // T=10000us
    timerAlarmEnable(timer); // Start Timer
  */
  //--- Counter setup for Rotary Encoder ---------------------
  pcnt_config_t pcnt_config_A;// structure for A
  pcnt_config_t pcnt_config_B;// structure for B
  //
  pcnt_config_A.pulse_gpio_num = ROTARY_ENCODER_A_PIN;
  pcnt_config_A.ctrl_gpio_num = ROTARY_ENCODER_B_PIN;
  pcnt_config_A.lctrl_mode = PCNT_MODE_REVERSE;
  pcnt_config_A.hctrl_mode = PCNT_MODE_KEEP;
  pcnt_config_A.channel = PCNT_CHANNEL_0;
  pcnt_config_A.unit = PCNT_UNIT_0;
  pcnt_config_A.pos_mode = PCNT_COUNT_INC;
  pcnt_config_A.neg_mode = PCNT_COUNT_DEC;
  pcnt_config_A.counter_h_lim = 10000;
  pcnt_config_A.counter_l_lim = -10000;
  //
  pcnt_config_B.pulse_gpio_num = ROTARY_ENCODER_B_PIN;
  pcnt_config_B.ctrl_gpio_num = ROTARY_ENCODER_A_PIN;
  pcnt_config_B.lctrl_mode = PCNT_MODE_KEEP;
  pcnt_config_B.hctrl_mode = PCNT_MODE_REVERSE;
  pcnt_config_B.channel = PCNT_CHANNEL_1;
  pcnt_config_B.unit = PCNT_UNIT_0;
  pcnt_config_B.pos_mode = PCNT_COUNT_INC;
  pcnt_config_B.neg_mode = PCNT_COUNT_DEC;
  pcnt_config_B.counter_h_lim = 10000;
  pcnt_config_B.counter_l_lim = -10000;
  //
  pcnt_unit_config(&pcnt_config_A);//Initialize A
  pcnt_unit_config(&pcnt_config_B);//Initialize B
  pcnt_counter_pause(PCNT_UNIT_0);
  pcnt_counter_clear(PCNT_UNIT_0);
  pcnt_counter_resume(PCNT_UNIT_0); //Start


    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println(WiFi.localIP());

    hub.config(F("MyDevices"), F("MPGSP-ESP32"));
    //hub.onBuild(build);
    hub.begin();
} 
// ********   END  SETUP   ******** //


// ******** ТЕЛО ПРОГРАММЫ ******** //
void loop() {
  mill = millis();
  
  readButtons();
  
  // ************** Выбор режима церрер ***************//
  if (SbClic) {
    SbClic = false;
    Serial.println("Режим ZEPPER");

#ifdef LCD_RUS
    goZepper();
#else
    goZepper();
#endif
  }
  // **************************************************//


  if ( SbLong) {
    SbLong = false;
    Serial.println("Режим STATIC");
    oldmemTimers = memTimers;
    isWorkStarted = 1;
    digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
    si5351.output_enable(SI5351_CLK0,1);

#ifndef TFT_ERR
    tftDisplay();
#endif
    readAnalogAndSetFreqInSetup();
    readDamp(currentEncoderPos);
    timMillis = millis();
  }

  // *********** Рабочий режим ***************//
  if (isWorkStarted == 1) {
    memTimers = setTimerLCD(memTimers);

    if (mill - prevUpdateDataIna > 1000 * 2) {
      readAnalogAndSetFreqInLoop();
      Data_ina219 = ina219.shuntCurrent() * 1000;
      prevUpdateDataIna = millis();
#ifndef TFT_ERR
      tftDisplay();
#endif
    }
  }


  // если значение экодера поменялось
  if (currentEncoderPos != newEncoderPos) {
    // если работа ещё не началась, то можем устанавливать время экспозиции, если началась, то мощность
    if (isWorkStarted == 0) {
      setTimer();
    } else if (isWorkStarted == 1) {
      // если работа началась, то можем редактировать потенциометром мощность
      processPotenciometr();
    }
    currentEncoderPos = newEncoderPos;
    readDamp(map(wiperValue, 0, 255, 0, 100));  // Значение с потенциометра в % и далее преобразовать в Вольты
#ifndef TFT_ERR
    tftDisplay();
#endif
  }
} // *************** E N D  L O O P ****************




// ***************** Функция Цеппера ****************
void goZepper() {
  int sst = 0;       // возьмём нулевой элемент массива
  bool fgen = false; // синус / меандр = true (отслеживание смены режима)

  do {
#ifdef DEBUG
    printStruct();
#endif
    Serial.println();

    if (Cicle.Freq > 0) {                      // Если в массиве частота не 0 то работаем по циклограмме
      // Режим генератора синус/меандр
      if (fgen != Cicle.ModeGen) {             // смена режима, пауза, Зуммер 5 секунд
        fgen = Cicle.ModeGen;
        Serial.println("Смена режима выхода");
        start_Buzzer();                        // Звуковой сигнал
        //testTFT(10 * 1000);
        delay(10000);
        Serial.println("Конец паузы 10 секунд");
        stop_Buzzer();
      }
      yield();
      tft.drawCentreString("Zepper is ON", 120, 275, 2); // Очистка экрана стирает вывод

      if (!fgen) {                              // *** Выход генератора - STATIC ***
        digitalWrite(PIN_RELE, LOW);            // Выход реле (NC)
        int power = 20;                         // Очки, половинная мощность (5 вольт) - 20%
        setResistance(map(power, 0, 100, 0, d_resis));
        Serial.print("U = ");
        Serial.print(power);
        Serial.println(" %");

        digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода
        if (Cicle.ModeSig == 1)
          Ad9833.setMode(MD_AD9833::MODE_SINE);
        if (Cicle.ModeSig == 2)
          Ad9833.setMode(MD_AD9833::MODE_SQUARE1);

        Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)Cicle.Freq);
        si5351.output_enable(SI5351_CLK0,1);
        setSI5351Freq(Cicle.Freq);
        freqDisp = Cicle.Freq;                    // Частота, вывести на дисплей
        readDamp(power);                          // Получить уровень мощности
        
#ifdef DEBUG        
        Serial.print("Частота ");
        Serial.print((float)Cicle.Freq / 1000, 3);
        Serial.println(" KHz");
     
        printModeSigToSerial(Cicle.ModeSig);

        // в этом месте вывести (продублировать) бы информацию на дисплей

        Serial.print("ЖдёM ");
        printTimeSerial(Cicle.Exposite);
#endif 

#ifndef TFT_ERR
        tftDisplay();
#endif
        // Выводим часы по времени экспозиции, опроса кнопок не реализовано (временно)
        testTFT(Cicle.Exposite * 1000);

#ifndef TFT_ERR
        tftDisplay();
#endif
        //delay(Cicle.Exposite * 1000);         // Выдержка экспозиции частоты

        if (Cicle.Pause != 0) {                 // Отработаем паузу, если она есть
          Serial.print("Пауза ");
          printTimeSerial(Cicle.Pause);
          digitalWrite(ON_OFF_CASCADE_PIN, LOW); // Разрешение выхода
          si5351.output_enable(SI5351_CLK0,0);
          freqDisp = 0;
          testTFT(Cicle.Pause * 1000);

#ifndef TFT_ERR
          tftDisplay();
#endif

          digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода
          si5351.output_enable(SI5351_CLK0,1);
          freqDisp = Cicle.Freq;
        }
      } else {                                  // *** Режим меандра ***
        digitalWrite(PIN_RELE, HIGH);            // Выход реле (NO)
        int power = 50;                          // ZEPPER, полная мощность (12 вольт) - УТОЧНИТЬ!!!
        setResistance(map(power, 0, 100, 0, d_resis));
        Serial.print("U = ");
        Serial.print(power);
        Serial.println(" %");

        digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода

        if (Cicle.ModeSig == 1)
          Ad9833.setMode(MD_AD9833::MODE_SINE);
        if (Cicle.ModeSig == 2)
          Ad9833.setMode(MD_AD9833::MODE_SQUARE1);

        Ad9833.setFrequency(MD_AD9833::CHAN_0, (float)Cicle.Freq);
        si5351.output_enable(SI5351_CLK0,1);
        setSI5351Freq(Cicle.Freq);
        freqDisp = Cicle.Freq;      // Частота, вывести на дисплей
       
        Serial.print("Частота ");
        Serial.print((float)Cicle.Freq / 1000, 3);
        Serial.println(" KHz");
        readDamp(power);    // Получить уровень мощности
        printModeSigToSerial(Cicle.ModeSig);

        // Здесть может быть вывод на TFT экран
#ifndef TFT_ERR
        tftDisplay();
#endif
        Serial.println("ЖдёM ");
        printTimeSerial(Cicle.Exposite);
        testTFT(Cicle.Exposite * 1000);

#ifndef TFT_ERR
        tftDisplay();
#endif
        // delay(Cicle.Exposite * 1000);        // Выдержка экспозиции частоты

        if (Cicle.Pause != 0) {                 // Отработаем паузу, если она есть
          Serial.print("Пауза ");
          printTimeSerial(Cicle.Pause);
          digitalWrite(ON_OFF_CASCADE_PIN, LOW); // Разрешение выхода
          si5351.output_enable(SI5351_CLK0,0);
          testTFT(Cicle.Pause * 1000);
          freqDisp = 0;
          
#ifndef TFT_ERR
          tftDisplay();
#endif
          // delay(Cicle.Pause * 1000);
          digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода
          si5351.output_enable(SI5351_CLK0,1);
        }
      }
      sst += 1;
      setStructure(sst); // Заполним структуру из массивов

    }
  } while (Cicle.Freq > 0 );

  printStruct();
  Serial.println("Сеанс окончен");
  digitalWrite(ON_OFF_CASCADE_PIN, LOW); // Разрешение выхода OFF
  si5351.output_enable(SI5351_CLK0,0);
  digitalWrite(PIN_RELE, LOW);            // Выход реле (NC)
  sst = 0;

  // В структуру восстановить первую запись
  setStructure(sst);

  isWorkStarted = false; // рабочий режим окончен

  // Почистим строку экрана
  yield();
  tft.drawCentreString("                       ", 120, 275, 2);
  yield();
  tft.drawCentreString("Zepper is OFF", 120, 275, 2);
  delay(10);
}
// ******** Конец процедуры ZEPPER ********* //


void setStructure(int strc) {
  Cicle.ModeGen  = zepDbModeGen[strc];
  Cicle.ModeSig  = zepDbModeSig[strc];
  Cicle.Freq     = zepDbFreq[strc];
  Cicle.Exposite = zepDbExpo[strc];
  Cicle.Pause    = zepDbPause[strc];
}

void readDamp(int pw) {

#ifdef DEBUG
  Serial.print("Разрешение выхода = ");
  bool on_off = digitalRead(ON_OFF_CASCADE_PIN);
  if (on_off) {
    Serial.println("Включено");
  }
  else {
    Serial.println("Выключено");
  }

  Serial.print("Выход сигнала на разъём ");
  if (digitalRead(PIN_RELE)) {
    Serial.println("ZEPPER");
  } else {
    Serial.println("STATICS");
  }
  Serial.print("Мощность выхода = ");
  Serial.print((float)map(pw, 0, 100, 0, 24000) / 1000, 2);
  Serial.println(" Вольт");

#endif
}

void printTimeSerial(int t_sec) {
  int tw = t_sec / 60;
  if (tw >= 1) {
    // Минуты
    if (tw == 1) {
      Serial.print(tw);
      Serial.println(" минуту");
    }
    if (tw >= 2 && tw < 5) {
      Serial.print(tw);
      Serial.println(" минуты");
    }
    if (tw >= 5 && tw < 101) {
      Serial.print(tw);
      Serial.println(" минут");
    }

  } else {
    // Секунды (до 600, здесь не потребуется, на будущее)
    tw = t_sec;
    if (tw == 1 || tw == 101 || tw == 201 || tw == 301 || tw == 401 || tw == 501 )
    {
      Serial.print(tw);
      Serial.println(" секунда");
    }
    if ((tw >= 2 && tw < 5) || (tw >= 102 && tw < 105) || (tw >= 202 && tw < 205) || (tw >= 302 && tw < 305) || (tw >= 402 && tw < 405) || (tw >= 502 && tw < 505))
    {
      Serial.print(tw);
      Serial.println(" секунды");
    }
    if ((tw >= 5 && tw < 101) || (tw >= 105 && tw < 201) || (tw >= 205 && tw < 301) || (tw >= 305 && tw < 401) || (tw >= 405 && tw < 501) || (tw >= 505 && tw < 601))
    {
      Serial.print(tw);
      Serial.println(" секунд");
    }
  }
  Serial.println();
}

void printModeSigToSerial(int m_sig) {
  Serial.print("Режим генератора - ");
  if (m_sig == 1) Serial.println("Синус");
  if (m_sig == 2) Serial.println("Меандр");
}


/*------- RUS fonts from UTF-8 to Windows-1251 -----*/
String utf8rus(String source)
{
  int i, k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 127) {
      switch (n) {
        case 208: {
            n = source[i]; i++;
            if (n == 129) {
              n = 192;  // перекодируем букву Ё
              break;
            }
            break;
          }
        case 209: {
            n = source[i]; i++;
            if (n == 145) {
              n = 193;  // перекодируем букву ё
              break;
            }
            break;
          }
      }
    }

    m[0] = n; target = target + String(m);
  }
  return target;
}


void setSI5351Freq(long siFreq) { //функция работа с синтезатором si5351
  if (!si5351_found) {
    return; // на всякий случай
  }
  if (siFreq < 8000) {
    freq = 8000; //допустимые рамки частот
  } if (siFreq > 160E6) {
    siFreq = 160E6;
  }
  si5351.set_freq(siFreq * SI5351_FREQ_MULT, SI5351_CLK0);
}

void readButtons(){
   Btn1.run();

   if (Btn1.read() == sbClick) {
    SbClic = true;
  }
  
  if (Btn1.read() == sbLong && !isWorkStarted) {
    SbLong = true;
  } 
}
