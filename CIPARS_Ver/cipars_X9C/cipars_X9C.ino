// Генератор для катушки Мишина на основе DDS AD9833

/*  06.04.2024 - добавлена отладка
    04.04.2024 - Версия консолидированная для Гитхаба
    https://github.com/UA6EM/MPGSP/tree/master/CIPARS_Ver/cipars_X9C

    03.04.2024 - Для цеппера выствлен режим сигнала Меандр
    31.03.2024 - Добавлен режим цеппера, вход по короткому нажатию кнопки
    25.02.2024 - Версия CIPARS для потенциометра  X9C, управление AD9833 напрямую

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

#define DEBUG       // Отладочная информация в порт Serial

#define SECONDS(x) ((x) * 1000UL)
#define MINUTES(x)  (SECONDS(x) * 60UL)
#define HOURS(x)  (MINUTES(x) * 60UL)
#define DAYS(x)   (HOURS(x) * 24UL)
#define WEEKS(x)  (DAYS(x) * 7UL)
unsigned long interval = MINUTES(1);
unsigned long oneMinute = MINUTES(1);
unsigned long timers = MINUTES(5); // время таймера 15, 30, 45 или 60 минут
unsigned long memTimers = 0; //здесь будем хранить установленное время таймера
unsigned long oldmemTimers = 0;
byte isWorkStarted = 0; // флаг запуска таймера

unsigned long timMillis = 0;
unsigned long oldMillis = 0;
unsigned long mill; // переменная под millis()
unsigned long prevCorrectTime = 0;
unsigned long prevReadAnalogTime = 0; // для отсчета 10 секунд между подстройкой частоты
unsigned long prevUpdateDataIna = 0; // для перерыва между обновлениями данных ina

#include <Wire.h>
#include <SPI.h>

#define LCD_RUS                // Замаркировать если не LCD_RUS вывод на дисплей
#ifdef LCD_RUS                 // Для использования русской кодировки дисплея LCD
#include "LCD_1602_RUS.h"      // https://github.com/UA6EM/LCD_1602_RUS (исправлена под ESP32)
LCD_1602_RUS lcd(0x27, 16, 2); // используемый дисплей (0x3F, 16, 2) адрес,символов в строке,строк.
#else
// здесь может быть код для вашего дисплея, для этого:
// добавьте ваш код для LCD в файле display.ino и в функции zepper()

#define I2C_ADDR 0x27
#include <LiquidCrystal_I2C.h>
//LiquidCrystal_I2C lcd(I2C_ADDR, 20, 4);
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);
#endif
//
#include "INA219.h"
INA219 ina219;

// PINS
#define CORRECT_PIN A7 // пин для внешней корректировки частоты.
#define ON_OFF_CASCADE_PIN 5 // для выключения выходного каскада
#define PIN_ENCODER1 6
#define PIN_ENCODER2 7
#define PIN_ENCODER3 3
#define PIN_ENCODER_BUTTON 8
#define PIN_ZUM 9
#define PIN_FSYNC 10
#define PIN_RELE 2

// пины потенциометра
#define PIN_CS 4
#define PIN_INC A1
#define PIN_UD A2

#define zFreq 2     // делитель интервала - секунда/2

unsigned int Data_ina219 = 0;

const int SINE = 0x2000;                    // определяем значение регистров AD9833 в зависимости от формы сигнала
const int SQUARE = 0x2020;                  // После обновления частоты нужно определить форму сигнала
// const int TRIANGLE = 0x2002;                // и произвести запись в регистр.
const float refFreq = 25000000.0;           // Частота кристалла на плате AD9833

long FREQ_MIN = 200000; // 200kHz
long FREQ_MAX = 500000; // 500kHz
long ifreq = FREQ_MIN;
long freq = FREQ_MIN;


const unsigned long availableTimers[] = {oneMinute * 15, oneMinute * 30, oneMinute * 45, oneMinute * 60};
const byte maxTimers = 4;
int timerPosition = 0;
// по умолчанию 50% потенциометра
int currentPotenciometrPercent = 50;

/********* используемые подпрограммы выносим сюда *********/

/*** Обработчик кнопки энкодера ***/
//------Cl_Btn----------------------
enum {sbNONE = 0, sbClick, sbLong}; /*состояние не изменилось/клик/долгое нажатие*/
class Cl_Btn {
  protected:
    const byte pin;
    byte state;
    bool bounce = 0;
    bool btn = 1, oldBtn;
    unsigned long past;
    const uint32_t time = 500 ;
    bool flag = 0;
    uint32_t past_flag = 0 ;
  public:
    Cl_Btn(byte p): pin(p) {}
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
        bounce = 0 ;
        oldBtn = btn;
        btn = newBtn;
        if (!btn && oldBtn) {
          flag = 1;
          past_flag = mill;
        }
        if (!oldBtn && btn && flag && mill - past_flag < time ) {
          flag = 0;
          state = sbClick;
        }
      }
      if (flag && mill - past_flag >= time ) {
        flag = 0;
        state = sbLong;
      }
    }
    byte read() {
      return state;
    }
};

Cl_Btn Btn1(PIN_ENCODER_BUTTON); //Экземпляр обработчика для кнопки энкодера

/******* Простой энкодер *******/
#include <util/atomic.h>        // для атомарности чтения данных в прерываниях
#include "RotaryEncoder.h"      // https://www.arduino.cc/reference/en/libraries/rotaryencoder/
RotaryEncoder encoder(PIN_ENCODER1, PIN_ENCODER2);

volatile int newEncoderPos; // новая позиция энкодера
static int currentEncoderPos = 0; // текущая позиция энкодера
/*** Обработчик прерывания для энкодера ***/
ISR(PCINT2_vect) {
  encoder.tick();
}

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

void resetPotenciometer() {
  // Понижаем сопротивление до 0%:
  analogWrite(PIN_UD, 0); // выбираем понижение
  digitalWrite(PIN_CS, LOW); // выбираем потенциометр X9C
  for (int i = 0; i < 100; i++) { // т.к. потенциометр имеет 100 доступных позиций
    analogWrite(PIN_INC, 0);
    delayMicroseconds(1);
    analogWrite(PIN_INC, 255);
    delayMicroseconds(1);
  }
  digitalWrite(PIN_CS, HIGH); /* запоминаем значение и выходим из режима настройки */
}

// Уровень percent - от 0 до 100% от максимума.
void setResistance(int percent) {
  resetPotenciometer();

  // Поднимаем сопротивление до нужного:
  analogWrite(PIN_UD, 255); // выбираем повышение
  digitalWrite(PIN_CS, LOW); // выбираем потенциометр X9C
  for (int i = 0; i < percent; i++) {
    analogWrite(PIN_INC, 0);
    delayMicroseconds(1);
    analogWrite(PIN_INC, 255);
    delayMicroseconds(1);
  }

  digitalWrite(PIN_CS, HIGH); /* запоминаем значение и выходим из режима настройки */
}

void processPotenciometr() {
  // если энкодер крутим по часовой
  if (newEncoderPos - currentEncoderPos > 0) {
    if (currentPotenciometrPercent >= 100) {
      currentPotenciometrPercent = 100;
    } else {
      currentPotenciometrPercent += 1;
    }
  } else if (newEncoderPos - currentEncoderPos < 0) {
    // если энкодер крутим против часовой
    if (currentPotenciometrPercent <= 1) {
      currentPotenciometrPercent = 1;
    } else {
      currentPotenciometrPercent -= 1;
    }
  }

  setResistance(currentPotenciometrPercent);
}

/*** Обработчик энкодера через ШИМ ***/
void startEncoder() {
  attachInterrupt(1, Encoder2, RISING );
  analogWrite(PIN_ENCODER3, 0x80); //установим на пине частоту
  //490 гц скважность 2
}
void Encoder2(void) { // процедура вызываемая прерыванием, пищим активным динамиком
  encoder.tick();
}

/********* Таймер обратного отсчёта экспозиции **********/
unsigned long  setTimerLCD(unsigned long timlcd) {
  if (millis() - timMillis >= 1000) {
    timlcd = timlcd - 1000;
    timMillis += 1000;
  }
  if (timlcd == 0) {
    timlcd = oldmemTimers;
    isWorkStarted = 0;
    lcd.setCursor(0, 3);
    lcd.print("     ЗАВЕРШЕНО!     ");
    digitalWrite(ON_OFF_CASCADE_PIN, LOW);
    start_Buzzer();
    delay(3000);
    stop_Buzzer();
    AD9833reset();
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
// AD9833 documentation advises a 'Reset' on first applying power.
void AD9833reset() {
  WriteRegister(0x100);   // Write '1' to AD9833 Control register bit D8.
  delay(10);
}

// Set the frequency and waveform registers in the AD9833.
void AD9833setFrequency(long frequency, int Waveform) {
  long FreqWord = (frequency * pow(2, 28)) / refFreq;
  int MSB = (int)((FreqWord & 0xFFFC000) >> 14);    //Only lower 14 bits are used for data
  int LSB = (int)(FreqWord & 0x3FFF);
  //Set control bits 15 ande 14 to 0 and 1, respectively, for frequency register 0
  LSB |= 0x4000;
  MSB |= 0x4000;
  WriteRegister(0x2100);
  WriteRegister(LSB);                  // Write lower 16 bits to AD9833 registers
  WriteRegister(MSB);                  // Write upper 16 bits to AD9833 registers.
  WriteRegister(0xC000);               // Phase register
  WriteRegister(Waveform);             // Exit & Reset to SINE, SQUARE or TRIANGLE
}

// *************************
// Display and AD9833 use different SPI MODES so it has to be set for the AD9833 here.
void WriteRegister(int dat) {
  SPI.setDataMode(SPI_MODE2);
  digitalWrite(PIN_FSYNC, LOW);           // Set FSYNC low before writing to AD9833 registers
  delayMicroseconds(10);              // Give AD9833 time to get ready to receive data.
  SPI.transfer(highByte(dat));        // Each AD9833 register is 32 bits wide and each 16
  SPI.transfer(lowByte(dat));         // bits has to be transferred as 2 x 8-bit bytes.
  digitalWrite(PIN_FSYNC, HIGH);          //Write done. Set FSYNC high
}

long readAnalogAndSetFreqInSetup() {
  int maxValue = 0;
  long freqWithMaxI = FREQ_MIN;
  long freqIncrease = 1000; // 1kHz
  int iterations = (FREQ_MAX - FREQ_MIN) / freqIncrease - 1; // (500000 - 200000) / 1000 - 1 = 199

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
    AD9833setFrequency(freq, SINE);
    delay(20);
  }

  ifreq = freqWithMaxI;

  AD9833setFrequency(ifreq, SINE);
  prevReadAnalogTime = millis();
}

/**** Подстройка частоты каждые 1-10 секунд относительно аналогового сигнала ***/
void readAnalogAndSetFreqInLoop() {
  unsigned long curr = millis();

  // если прошло N секунд с момента последней проверки
  if (curr - prevReadAnalogTime > 1000 * 5) { //выбор времени изменения частоты.1-10 сек.
    long availableDiff = 5000; // 1kHz-10kHz разница частот
    long freqIncrease = 500; // 100Hz-1kHz шаг увеличения частоты при сканировании

    int iterations = (availableDiff * 2) / freqIncrease - 1; // (10000 * 2) / 1000 - 1 = 19

    long minimalFreq = ifreq - availableDiff;
    if (minimalFreq < FREQ_MIN) {
      minimalFreq = FREQ_MIN;
    }
    // подаём на генератор минимальную частоту из диапазона +-10кГц
    AD9833setFrequency(minimalFreq, SINE);
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
      AD9833setFrequency(freq, SINE);
      delay(10);
    }

    ifreq = freqWithMaxI;

    AD9833setFrequency(ifreq, SINE);
    prevReadAnalogTime = millis();
  }
}

//************************** SETUP *************************/
void setup() {
  // настройки потенциометра
  // сначала настраиваем потенциометр
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_INC, OUTPUT);
  pinMode(PIN_UD, OUTPUT);
  digitalWrite(PIN_CS, HIGH);  // X9C в режиме низкого потребления
  analogWrite(PIN_INC, 255);
  analogWrite(PIN_UD, 255);
  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, LOW);

  delay(30);
  // сбрасываем потенциометр в 0%
  resetPotenciometer();
  // после сброса устанавливаем значение по умолчанию
  setResistance(currentPotenciometrPercent);

  // ждем секунду после настройки потенциометра
  delay(1000);

  Btn1.init();
  SPI.begin();
  Serial.begin(115200);

  pinMode(ON_OFF_CASCADE_PIN, OUTPUT);
  pinMode(PIN_ZUM, OUTPUT);
  pinMode(CORRECT_PIN, INPUT);

  digitalWrite(PIN_ZUM, LOW);
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);

  analogReference(INTERNAL);

  //lcd.begin();       // Зависит от версии библиотеки
  lcd.init();
  lcd.backlight();
  delay(10);
  ina219.begin(0x40); // (44) i2c address 64=0x40 68=0х44 исправлять и в ina219.h одновременно
  ina219.configure(0, 2, 12, 12, 7);      // 16S -8.51ms
  ina219.calibrate(0.100, 0.32, 16, 3.2);
  AD9833reset();                          // Ресет после включения питания
  delay(10);
  Serial.print("freq=");
  Serial.println(freq);
  // выставляем минимальную частоту для цикла определения максимального тока
  AD9833setFrequency(FREQ_MIN, SINE);
  delay(20);

  readAnalogAndSetFreqInSetup();

  Data_ina219 = ina219.shuntCurrent() * 1000;
  myDisplay();
  delay(1000);
  PCICR |= (1 << PCIE2); // инициализируем порты для энкодера
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);
  startEncoder();

  memTimers = availableTimers[0]; // выставляем 15 минут по умолчанию
}

// *** ТЕЛО ПРОГРАММЫ ***
void loop() {
  mill = millis();
  Btn1.run();

  if (Btn1.read() == sbClick) {
    Serial.println("Режим ZEPPER");
    setZepper();
  }

  if (Btn1.read() == sbLong) {
    oldmemTimers = memTimers;
    timMillis = millis();
    isWorkStarted = 1;
    readDamp(currentEncoderPos);
  }

  if (mill - prevUpdateDataIna > 1000 * 2) {
    Data_ina219 = ina219.shuntCurrent() * 1000;
    prevUpdateDataIna = millis();
  }

  myDisplay();

  if (isWorkStarted == 1) {
    memTimers = setTimerLCD(memTimers);
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    newEncoderPos = encoder.getPosition();
  }

  // если значение экодера поменялось
  if (currentEncoderPos != newEncoderPos) {
    // если работа ещё не началась, то можем устанавливать время
    if (isWorkStarted == 0) {
      setTimer();
    } else if (isWorkStarted == 1) {
      // если работа ещё началась, то можем редактировать потенциометр
      processPotenciometr();
    }
    currentEncoderPos = newEncoderPos;
    readDamp(currentEncoderPos);
  }
  readAnalogAndSetFreqInLoop();

} // *********** E N D  L O O P **************



// ************* Функция Цеппера *************
void setZepper() {
  digitalWrite(PIN_RELE, HIGH);
  int power = 5;   // Очки, половинная мощность (5 вольт)
  setResistance(map(power, 0, 12, 0, 100));
  Serial.print("U = ");
  Serial.println(map(power, 0, 12, 0, 100));

  long zepFreq = 473000;
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
  AD9833setFrequency(zepFreq, SQUARE);
  Serial.println("Частота 473 KHz");
  readDamp(power);

  lcd.setCursor(0, 0);
  lcd.print("  F - 473 KHz  ");
  lcd.setCursor(0, 1);
  lcd.print("ЖдёM 2-е Mинуты");
  delay(120000);
  zepFreq = 395000;
  AD9833setFrequency(zepFreq, SQUARE);
  Serial.println("Частота 395 KHz");
  readDamp(power);

  lcd.setCursor(0, 0);
  lcd.print("  F - 395 KHz  ");
  lcd.setCursor(0, 1);
  lcd.print("Ждём 2-е минуты");
  delay(120000);
  zepFreq = 403850;
  AD9833setFrequency(zepFreq, SQUARE);
  Serial.println("Частота 403.85 KHz");
  readDamp(power);

  lcd.setCursor(0, 0);
  lcd.print("F - 403.85 KHz ");
  lcd.setCursor(0, 1);
  lcd.print("Ждём 2-е минуты");
  delay(120000);
  zepFreq = 397600;
  AD9833setFrequency(zepFreq, SQUARE);
  Serial.println("Частота 397.6 KHz");
  readDamp(power);

  lcd.setCursor(0, 0);
  lcd.print(" F - 397.6 KHz ");
  lcd.setCursor(0, 1);
  lcd.print("Ждём 2-е минуты");
  delay(120000);

  power = 127;  // Электроды, полная мощность
  setResistance(power);
  digitalWrite(PIN_RELE, HIGH); // Переключим выход генератора на Электроды
  zepFreq = 30000;
  AD9833setFrequency(zepFreq, SQUARE);
  Serial.println("Частота 30 KHz");
  readDamp(power);

  lcd.setCursor(0, 0);
  lcd.print("   F - 30 KHz   ");
  lcd.setCursor(0, 1);
  lcd.print("  Ждём 7 минут  ");
  delay(420000);
  digitalWrite(ON_OFF_CASCADE_PIN, LOW);
  Serial.println("Перерыв 20 минут");
  readDamp(power);

  lcd.setCursor(0, 0);
  lcd.print("     IS OFF     ");
  lcd.setCursor(0, 1);
  lcd.print(" Отдых 20 минут ");
  delay(1200000);
  digitalWrite(ON_OFF_CASCADE_PIN, HIGH);
  zepFreq = 30000;
  AD9833setFrequency(zepFreq, SQUARE);
  Serial.println("Частота 30 KHz");
  readDamp(power);

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
  digitalWrite(PIN_RELE, LOW); // Переключим выход генератора на катушку
  readDamp(power);
}


void readDamp(int pw) {
#ifdef DEBUG
  Serial.print("Разрешение выхода = ");
  Serial.println(digitalRead(ON_OFF_CASCADE_PIN));
  Serial.print("Выходной разъём = ");
  if (digitalRead(PIN_RELE)) {
    Serial.println("ZEPPER");
    Serial.print("U = ");
    Serial.println(map(pw, 0, 12, 0, 100));
  } else {
    Serial.println("STATICA");
    Serial.print("Мощность выхода = ");
    Serial.println(currentEncoderPos);
  }
#endif
}
