// Генератор для катушки Мишина на основе DDS AD9833 для ESP32 экран LCD

// Partition Scheme: NO OTA (2MB APP, 2MB SPIFFS)
// При компиляции, в настройках IDE оключить все уведомления
// иначе вылетит по ошибке на sqlite3
// Важно!!! Измените настройки на Вашу WIFI сеть в конфигурационном файле
// Для библиотеки TFT_eSPI приведен конфигурационный файл  для ILI9341

//                    Использование GITHUB
// 1. Клонируйте проект: git clone https://github.com/UA6EM/VESTA
// 2. Исключите конфигурационный файл из индекса:
//    git update-index --assume-unchanged esp32gsp/config.h
//  (для отмены git update-index --no-assume-unchanged your_file)
// 3. Исправьте конфигурацию в соответсвии с вашей сетью
//    Изменения в этом файле на локальном компьютере теперь
//    не попадут на GITHUB

/*
   Версия:
    04.04.2024 - проверена работа экрана LCD

    используемые библиотеки:
    Ai_Esp32_Rotary_Encoder-1.6.0         - https://www.arduino.cc/reference/en/libraries/ai-esp32-rotary-encoder/
    LiquidCrystal_I2C-master версии 1.1.4 - https://github.com/UA6EM/LiquidCrystal_I2C
    LCD_1602_RUS-master версии 1.0.5      - https://github.com/UA6EM/LCD_1602_RUS
    Ticker версии 2.0.                    - https://www.arduino.cc/reference/en/libraries/ticker/
    MCP4xxxx-ua6em версии 0.1             - https://github.com/UA6EM/MCP4xxxx
    AD9833-mpgsp версии 0.4.0             - https://github.com/UA6EM/AD9833/tree/mpgsp
    https://github.com/madhephaestus/ESP32Encoder
*/

// Определения
//#define DEBUG                          // Замаркировать если не нужны тесты
#define LCD_RUS                          // Замаркировать, если скетч для пользователя CIPARS
#define SECONDS(x) ((x)*1000UL)
#define MINUTES(x) (SECONDS(x) * 60UL)
#define HOURS(x) (MINUTES(x) * 60UL)
#define DAYS(x) (HOURS(x) * 24UL)
#define WEEKS(x) (DAYS(x) * 7UL)
#define ON_OFF_CASCADE_PIN 32  // Для выключения выходного каскада
#define PIN_ZUM 33
#define CORRECT_PIN A3         // Пин для внешней корректировки частоты.

//ROTARY ENCODER
#define ROTARY_ENCODER_A_PIN 34
#define ROTARY_ENCODER_B_PIN 35
#define PIN_ENC_BUTTON 25
#define ROTARY_ENCODER_BUTTON_PIN 36
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
//depending on your encoder - try 1,2 or 4 to get expected behaviour
#define ROTARY_ENCODER_STEPS 1
//#define ROTARY_ENCODER_STEPS 2
//#define ROTARY_ENCODER_STEPS 4
#define  MCP4151MOD       // используем библиотеку c с разрешением 255 единиц (аналог MCP4151)

#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"
//#include "config.h"

#include <Ticker.h>
Ticker my_encoder;
float encPeriod = 0.05;

#define PIN_RELE 2

//AD9833
//#define AD9833_MISO 12
#define AD9833_MOSI 13
#define AD9833_SCK 14
#define AD9833_CS 15

//LCD1602_I2C OR LCD2004_I2C AND INA219
#define I2C_SDA     21    // LCD1602 SDA
#define I2C_SCK     22    // LCD1602 SCK

//MCP41010
#define  MCP41x1_SCK   18 // Define SCK pin for MCP4131 or MCP41010
#define  MCP41x1_MOSI  23 // Define MOSI pin for MCP4131 or MCP41010
#define  MCP41x1_MISO  19 // Define MISO pin for MCP4131 or MCP41010
#define  MCP41x1_CS    5  // Define chipselect pin for MCP41010

#define zFreq 2           // Делитель интервала - секунда/2

// Глобальные переменные
unsigned long interval = MINUTES(1);
unsigned long oneMinute = MINUTES(1);
unsigned long timers = MINUTES(5);  // время таймера 15, 30, 45 или 60 минут
unsigned long memTimers = 0;        //здесь будем хранить установленное время таймера
unsigned long oldmemTimers = 0;
byte isWorkStarted = 0;  // флаг запуска таймера

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
const unsigned long freqSPI = 250000;  // Частота только для HW SPI AD9833
// UNO SW SPI = 250kHz
const unsigned long availableTimers[] = { oneMinute * 15, oneMinute * 30, oneMinute * 45, oneMinute * 60 };
const byte maxTimers = 4;
int timerPosition = 0;
volatile int newEncoderPos;            // Новая позиция энкодера
static int currentEncoderPos = 0;      // Текущая позиция энкодера
volatile  int d_resis = 127;
bool SbLong = false;

#ifndef LCD_RUS
#define I2C_ADDR 0x3F //0x27
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(I2C_ADDR, 20, 4);
#else
#define I2C_ADDR 0x3F
#include <LCD_1602_RUS.h>
LCD_1602_RUS lcd(I2C_ADDR, 16, 2);
#endif

#include <Wire.h>
#include <SPI.h>

#include <MCP4xxxx.h>  // https://github.com/UA6EM/MCP4xxxx
MCP4xxxx Potentiometer(MCP41x1_CS, MCP41x1_MOSI, MCP41x1_SCK, 250000UL, SPI_MODE0);
//MCP4xxxx Potentiometer(MCP41x1_CS);


#include "INA219.h"
INA219 ina219;

// по умолчанию 50% потенциометра
int currentPotenciometrPercent = 127;


//--------------- Create an AD9833 object ----------------
#include <AD9833.h>  // Пробуем новую по ссылкам в README закладке
//AD9833 AD(10, 11, 13);     // SW SPI over the HW SPI pins (UNO);
//AD9833 Ad9833(AD9833_CS);  // HW SPI Defaults to 25MHz internal reference frequency
AD9833 Ad9833(AD9833_CS, AD9833_MOSI, AD9833_SCK); // SW SPI speed 250kHz


/******* Простой энкодер *******/
//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

void rotary_onButtonClick() {
  Serial.print("maxTimers = ");
  Serial.println(maxTimers);
  static unsigned long lastTimePressed = 0;
  //ignore multiple press in that time milliseconds
  if (millis() - lastTimePressed < 500) {
    return;
  }
  lastTimePressed = millis();
  Serial.print("button pressed ");
  Serial.print(millis());
  Serial.println(" milliseconds after restart");
}

void rotary_loop() {
  //dont print anything unless value changed
  if (rotaryEncoder.encoderChanged()) {
    Serial.print("Value: ");
    Serial.println(rotaryEncoder.readEncoder());
  }
  if (rotaryEncoder.isEncoderButtonClicked()) {
    rotary_onButtonClick();
  }
}

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

//    *** Используемые подпрограммы выносим сюда ***   //

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


// функция выбора времени работы
void setTimer() {
  // если энкодер крутим по часовой
  if (newEncoderPos - currentEncoderPos > 0) {
    if (timerPosition == maxTimers - 1) {
      timerPosition = 0;
    } else {
      timerPosition += 1;
    }
  } else if (newEncoderPos - currentEncoderPos < 0) {
    // если энкодер крутим против часовой
    if (timerPosition == 0) {
      timerPosition = maxTimers - 1;
    } else {
      timerPosition -= 1;
    }
  }
  memTimers = availableTimers[timerPosition];
}

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
      currentPotenciometrPercent += 1;
      wiperValue += 1;
    }
  } else if (newEncoderPos - currentEncoderPos < 0) {
    // если энкодер крутим против часовой
    if (currentPotenciometrPercent <= 1) {
      currentPotenciometrPercent = 1;
      wiperValue = 1;
    } else {
      currentPotenciometrPercent -= 1;
      wiperValue -= 1;
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
    lcd.setCursor(0, 1);
#ifdef LCD_RUS
    lcd.print("    СТОП!     ");
#else
    lcd.print("    STOP!     ");
#endif
    digitalWrite(ON_OFF_CASCADE_PIN, LOW);
    start_Buzzer();
    delay(3000);
    stop_Buzzer();
    lcd.setCursor(0, 1);
     lcd.print("              ");
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
    Ad9833.setFrequency((float)freq, AD9833_SINE);
    delay(20);
  }
  ifreq = freqWithMaxI;
  // подаём частоту на генератор
  Ad9833.setFrequency((float)ifreq, AD9833_SINE);
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
    Ad9833.setFrequency((float)minimalFreq, AD9833_SINE);
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
      Ad9833.setFrequency((float)freq, AD9833_SINE);
      delay(10);
    }
    ifreq = freqWithMaxI;
    Ad9833.setFrequency((float)ifreq, AD9833_SINE);
    prevReadAnalogTime = millis();
  }
}

// *** Вывод на дисплей ***
void myDisplay() {
  // 1 строка
  lcd.setCursor(0, 0);
  if (!isWorkStarted) {
#ifdef LCD_RUS
    lcd.print("Время-");
#else
    lcd.print("Times-");
#endif
    lcd.print(memTimers / 60000);
    if (memTimers / 60000 > 0) {
#ifdef LCD_RUS
      lcd.print(" мин. ");
#else
      lcd.print(" min. ");
#endif
    } else {
#ifdef LCD_RUS
      lcd.print("0 мин. ");
#else
      lcd.print("0 min. ");
#endif
    }
  } else {
    lcd.print("T-");
    if (memTimers > 60000) {
      // если больше минуты, то показываем минуты
      lcd.print(memTimers / 1000 / 60);
#ifdef LCD_RUS
      lcd.print("мин.");
#else
      lcd.print("min.");
#endif
    } else {
      // если меньше минуты, то показываем секунды
      lcd.print(memTimers / 1000);
#ifdef LCD_RUS
      lcd.print("сек.");
#else
      lcd.print("sek.");
#endif
    }
    lcd.print(" U=");
#ifdef MCP4151MOD  // можно замапить в реальный % выходного сигнала
    lcd.print(map(currentPotenciometrPercent, 1, 255, 1, 100));
#else
    lcd.print(map(currentPotenciometrPercent, 1, 127, 1, 100));
#endif
    lcd.print("%  ");
  }

  // 2 строка
  lcd.setCursor(0, 1);
  lcd.print("F=");
  //lcd.setCursor(3, 1);                   //1 строка 7 позиция
  float freq_tic = ifreq;
  float kHz = freq_tic / 1000;
  lcd.print(kHz, 0);
  lcd.print("kHz");

  // 2 строка
  lcd.setCursor(9, 1);
  lcd.print("I=");
  lcd.setCursor(11, 1);
  lcd.print(Data_ina219 * 2);
  lcd.print("ma");
}


//************************** SETUP *************************/
void setup() {
  Serial.begin(115200);
  Serial.println("START");

  // lcd.begin();  // Зависит от версии библиотеки
  lcd.init();     //

  lcd.backlight();
  delay(1000);

  // сбрасываем потенциометр в 0%
  resetPotenciometer();
  // после сброса устанавливаем значение по умолчанию
  setResistance(currentPotenciometrPercent);

  // ждем секунду после настройки потенциометра
  delay(1000);

  Btn1.init();

  pinMode(ON_OFF_CASCADE_PIN, OUTPUT);
  pinMode(PIN_ZUM, OUTPUT);
  pinMode(CORRECT_PIN, INPUT);

  digitalWrite(PIN_ZUM, LOW);
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);


  ina219.begin(0x40);                 // (44) i2c address 64=0x40 68=0х44 исправлять и в ina219.h одновременно
  ina219.configure(0, 2, 12, 12, 7);  // 16S -8.51ms
  ina219.calibrate(0.100, 0.32, 16, 3.2);

  SPI.begin();
  // This MUST be the first command after declaring the AD9833 object
  Ad9833.begin();              // The loaded defaults are 1000 Hz SINE_WAVE using REG0
  Ad9833.reset();              // Ресет после включения питания
  Ad9833.setSPIspeed(freqSPI); // Частота SPI для AD9833 установлена 4 MHz
  Ad9833.setWave(AD9833_OFF);  // Turn OFF the output
  delay(10);
  Ad9833.setWave(AD9833_SINE);  // Turn ON and freq MODE SINE the output

  // выставляем минимальную частоту для цикла определения максимального тока
  Ad9833.setFrequency((float)FREQ_MIN, AD9833_SINE);

  Serial.print("freq=");
  Serial.println(FREQ_MIN);

  Data_ina219 = ina219.shuntCurrent() * 1000;
  myDisplay();
  delay(100);

  memTimers = availableTimers[0];  // выставляем 15 минут по умолчанию
#ifdef DEBUG
  testMCP41010();
#endif
  wiperValue = d_resis / 2;
  //currentEncoderPos = wiperValue;
  Potentiometer.writeValue(wiperValue);  // Set MCP4131 or MCP4151 to mid position
}  //******** END SETUP ********//


// *** ТЕЛО ПРОГРАММЫ ***
void loop() {
  mill = millis();
  Btn1.run();

  if (Btn1.read() == sbClick) {
    Serial.println("Режим ZEPPER");
#ifdef LCD_RUS
    setZepper();
#else
    setZepper1();
#endif
  }

  if (Btn1.read() == sbLong && !isWorkStarted) {
    SbLong = true;
  }

  if (mill - prevUpdateDataIna > 1000 * 2) {
    readAnalogAndSetFreqInLoop();
    Data_ina219 = ina219.shuntCurrent() * 1000;
    prevUpdateDataIna = millis();
  }

  myDisplay();

  if ( SbLong) {
    SbLong = false;
    oldmemTimers = memTimers;
    isWorkStarted = 1;
    digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
    myDisplay();
    readAnalogAndSetFreqInSetup();
    readDamp(currentEncoderPos);
    timMillis = millis();
  }

  if (isWorkStarted == 1) {
    memTimers = setTimerLCD(memTimers);
  }

  //  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
  //    newEncoderPos = encoder.getPosition();
  //  }

  // если значение экодера поменялось
  if (currentEncoderPos != newEncoderPos) {
    // если работа ещё не началась, то можем устанавливать время
    if (isWorkStarted == 0) {
      setTimer();
    } else if (isWorkStarted == 1) {
      // если работа ещё не началась, то можем редактировать потенциометр
      processPotenciometr();
    }
    currentEncoderPos = newEncoderPos;
    readDamp(currentEncoderPos);
  }
  // readAnalogAndSetFreqInLoop();

} // *************** E N D  L O O P ****************




// ***************** Функция Цеппера ****************
void setZepper() {
  digitalWrite(PIN_RELE, HIGH);
  int power = 5;   // Очки, половинная мощность (5 вольт)
  setResistance(map(power, 0, 12, 0, 100));
  Serial.print("U = ");
  Serial.println(map(power, 0, 12, 0, 100));

  long zepFreq = 473000;
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 473 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("  F - 473 KHz  ");
  lcd.setCursor(0, 1);
  lcd.print("ЖдёM 2-е Mинуты");
  delay(120000);
  zepFreq = 395000;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 395 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("  F - 395 KHz  ");
  lcd.setCursor(0, 1);
  lcd.print("Ждём 2-е минуты");
  delay(120000);
  zepFreq = 403850;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 403.85 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("F - 403.85 KHz ");
  lcd.setCursor(0, 1);
  lcd.print("Ждём 2-е минуты");
  delay(120000);
  zepFreq = 397600;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 397.6 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print(" F - 397.6 KHz ");
  lcd.setCursor(0, 1);
  lcd.print("Ждём 2-е минуты");
  delay(120000);

  start_Buzzer(); // Звуковой сигнал взять электроды
  delay(5000);
  stop_Buzzer();

  power = 12;  // Электроды, полная мощность
  setResistance(map(power, 0, 12, 0, 100));

  digitalWrite(PIN_RELE, HIGH); // Переключим выход генератора на Электроды
  zepFreq = 30000;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 30 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("   F - 30 KHz   ");
  lcd.setCursor(0, 1);
  lcd.print("  Ждём 7 минут  ");
  delay(420000);
  digitalWrite(ON_OFF_CASCADE_PIN, LOW);
  Serial.println("Перерыв 20 минут");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("     IS OFF     ");
  lcd.setCursor(0, 1);
  lcd.print(" Отдых 20 минут ");
  delay(1200000);
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
  zepFreq = 30000;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 30 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("   F - 30 KHz   ");
  lcd.setCursor(0, 1);
  lcd.print("  Ждём 7 минут  ");
  delay(420000);
  digitalWrite(ON_OFF_CASCADE_PIN, LOW);
  Serial.println("Сеанс окончен");

  lcd.setCursor(0, 0);
  lcd.print(" Сеанс окончен  ");
  lcd.setCursor(0, 1);
  lcd.print("Выключите прибор");
  delay(5000);
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  Ad9833.setFrequency(FREQ_MIN, AD9833_SINE);
  digitalWrite(PIN_RELE, LOW); // Переключим выход генератора на катушку
  readDamp(map(power, 0, 12, 0, 100));
}

void setZepper1() {
  digitalWrite(PIN_RELE, HIGH);
  int power = 5;   // Очки, половинная мощность (5 вольт)
  setResistance(map(power, 0, 12, 0, 100));
  Serial.print("U = ");
  Serial.println(map(power, 0, 12, 0, 100));

  long zepFreq = 473000;
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 473 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("  F - 473 KHz  ");
  lcd.setCursor(0, 1);
  lcd.print(" Wait 2 minutes");
  delay(120000);
  zepFreq = 395000;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 395 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("  F - 395 KHz  ");
  lcd.setCursor(0, 1);
  lcd.print(" Wait 2 minutes");
  delay(120000);
  zepFreq = 403850;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 403.85 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("F - 403.85 KHz ");
  lcd.setCursor(0, 1);
  lcd.print(" Wait 2 minutes");
  delay(120000);
  zepFreq = 397600;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 397.6 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print(" F - 397.6 KHz ");
  lcd.setCursor(0, 1);
  lcd.print(" Wait 2 minutes");
  delay(120000);

  start_Buzzer(); // Звуковой сигнал взять электроды
  delay(5000);
  stop_Buzzer();

  power = 12;  // Электроды, полная мощность
  setResistance(map(power, 0, 12, 0, 100));
  digitalWrite(PIN_RELE, HIGH); // Переключим выход генератора на Электроды

  zepFreq = 30000;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Частота 30 KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("Frequency 30KHz");
  lcd.setCursor(0, 1);
  lcd.print(" Wait 7 minutes ");
  delay(420000);
  digitalWrite(ON_OFF_CASCADE_PIN, LOW);
  Serial.println("Перерыв 20 минут");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("     IS OFF     ");
  lcd.setCursor(0, 1);
  lcd.print("Wait  20 minutes");
  delay(1200000);
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
  zepFreq = 30000;
  Ad9833.setFrequency(zepFreq, AD9833_SQUARE1);
  Serial.println("Frequency 30KHz");
  readDamp(map(power, 0, 12, 0, 100));

  lcd.setCursor(0, 0);
  lcd.print("Frequency 30 KHz");
  lcd.setCursor(0, 1);
  lcd.print(" Wait 7 minutes ");
  delay(420000);
  digitalWrite(ON_OFF_CASCADE_PIN, LOW);
  Serial.println("Сеанс окончен");
  lcd.setCursor(0, 0);
  lcd.print(" Session  over  ");
  lcd.setCursor(0, 1);
  lcd.print("Turn off  device");
  delay(5000);
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  Ad9833.setFrequency(FREQ_MIN, AD9833_SINE);
  digitalWrite(PIN_RELE, LOW); // Переключим выход генератора на катушку
  readDamp(map(power, 0, 12, 0, 100));
}


void readDamp(int pw) {
#ifdef DEBUG
  Serial.print("Разрешение выхода = ");
  Serial.println(digitalRead(ON_OFF_CASCADE_PIN));
  Serial.print("Выходной разъём = ");
  if (digitalRead(PIN_RELE)) {
    Serial.println("ZEPPER");
    Serial.print("U = ");
    Serial.println(pw);
  } else {
    Serial.println("STATIC");
    Serial.print("Мощность выхода = ");
    Serial.println(currentEncoderPos);
  }
#endif
}


/*
   G1 - TX
   G2 - PIN_RELE 2
   G3 - RX
   g4 -
   G5 - MCP41x1_CS    5            // Define chipselect pin for MCP41010
   G6 -
   G7 -
   G8 -
   G9 -
   G10 -
   G11 -
   G12 - AD9833_MISO 12
   G13 - AD9833_MOSI 13
   G14 - AD9833_SCK  14
   G15 - AD9833_CS   15
   G16 -
   G17 -
   G18 - MCP41x1_SCK   18           // Define SCK pin for MCP41010
   G19 - MCP41x1_MISO  19           // Define MISO pin for MCP4131 or MCP41010
   G20 -
   G21 - SDA // LCD, INA219
   G22 - SCK // LCD, INA219
   G23 - MCP41x1_MOSI   23          // Define MOSI pin for MCP4131 or MCP41010
   G24 -
   G25 - PIN_ENC_BUTTON 25
   G26 -
   G27 -
   G28 -
   G29 -
   G30 -
   G31 -
   G32 - ON_OFF_CASCADE_PIN
   G33 - PIN_ZUM 33
   G34 - ROTARY_ENCODER_A_PIN 34
   G35 - ROTARY_ENCODER_B_PIN 35
   G36 - ROTARY_ENCODER_BUTTON_PIN 36

   G39 - CORRECT_PIN A3 (ADC3)  SENS_IMPLOSION



*/
