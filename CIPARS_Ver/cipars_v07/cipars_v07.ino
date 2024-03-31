// Генератор для катушки Мишина на основе DDS AD9833

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

#define MCP4151MOD      // снять ремарки если используем библиотеку MCP4151(SPI у неё 16 битный)
#include <Wire.h>
#include <SPI.h>


const int chipSelect = 4;  // Define chipselect pin for MCP4131 or MCP4151
unsigned int wiperValue;   // variable to hold wipervalue for MCP4131 or MCP4151

#ifndef  MCP4151MOD
#include <MCP4131.h>       // https://github.com/UA6EM/Arduino-MCP4131/tree/mpgsp
MCP4131 Potentiometer(chipSelect);
#else
#include <MCP4151.h>       // https://github.com/UA6EM/MCP4151/tree/mpgsp
MCP4151 Potentiometer(chipSelect);
#endif

#include <LCD_1602_RUS.h>      // https://github.com/ssilver2007/LCD_1602_RUS
LCD_1602_RUS lcd(0x3F, 16, 2); // используемый дисплей (0x3F, 16, 2) адрес,символов в строке,строк.
//
#include "INA219.h"
INA219 ina219;


// PINS
//      chipSelect = 4 MCP4151 (SPI)

// SPI  SCK =  13
//      MOSI = 11
//      MISO = 12
//      SS   = 10

#define ON_OFF_CASCADE_PIN 5 // для выключения выходного каскада
#define PIN_ENCODER1 6
#define PIN_ENCODER2 7
#define PIN_ENCODER3 3
#define PIN_ENCODER_BUTTON 8
#define PIN_ZUM 9
#define PIN_FSYNC 10
// пины потенциометра
#define PIN_CS  A0
#define PIN_INC A1
#define PIN_UD  A2
#define CORRECT_PIN A7       // пин для внешней корректировки частоты.


#define zFreq 2     // делитель интервала - секунда/2

unsigned int Data_ina219 = 0;

const int SINE = 0x2000;                   // определяем значение регистров AD9833 в зависимости от формы сигнала
// const int SQUARE = 0x2020;              // После обновления частоты нужно определить форму сигнала
// const int TRIANGLE = 0x2002;            // и произвести запись в регистр.
const float refFreq = 25000000.0;          // Частота кристалла на плате AD9833

long FREQ_MIN = 200000; // 200kHz
long FREQ_MAX = 500000; // 500kHz
long ifreq = FREQ_MIN;
long freq = FREQ_MIN;
const unsigned long freqSPI = 4000000;

const unsigned long availableTimers[] = {oneMinute * 15, oneMinute * 30, oneMinute * 45, oneMinute * 60};
const byte maxTimers = 4;
int timerPosition = 0;
// по умолчанию 50% потенциометра
int currentPotenciometrPercent = 50;


#include <AD9833.h>             // https://github.com/UA6EM/AD9833-Library-Arduino/tree/mpgsp
//--------------- Create an AD9833 object ----------------
// Note, SCK and MOSI must be connected to CLK and DAT pins on the AD9833 for SPI
// -----      AD9833 ( FNCpin, referenceFrequency = 25000000UL )
//AD9833 gen(FNC_PIN);            // Defaults to 25MHz internal reference frequency
//AD9833 Ad9833(PIN_FSYNC, freqSPI);// Defaults to 25MHz internal reference frequency
AD9833 Ad9833(10,11,13);

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
#include <RotaryEncoder.h>      // https://www.arduino.cc/reference/en/libraries/rotaryencoder/
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

  // MCP4131
  wiperValue = 1;
  Potentiometer.writeValue(wiperValue);      // Set MCP4151 to position 1
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

  for (int i = 0; i < percent; i++) {
    wiperValue = i;
  }
  Potentiometer.writeValue(wiperValue);      // Set MCP4151
}

void processPotenciometr() {
  // если энкодер крутим по часовой
  if (newEncoderPos - currentEncoderPos > 0) {
    if (currentPotenciometrPercent >= 100) {
      currentPotenciometrPercent = 100;
      wiperValue = 100;
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
  Potentiometer.writeValue(wiperValue);      // Set MCP4131 to mid position
  Serial.print("wiperValue = ");
  Serial.println(wiperValue);
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
    //AD9833reset();
    //Ad9833.reset();
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
/*
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
*/

void /*long*/ readAnalogAndSetFreqInSetup() {
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
    //AD9833setFrequency(freq, SINE);
    Ad9833.setFrequency((float)freq);
    delay(20);
  }

  ifreq = freqWithMaxI;

  //AD9833setFrequency(ifreq, SINE);
  Ad9833.setFrequency((float)ifreq);
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
    //AD9833setFrequency(minimalFreq, SINE);
    Ad9833.setFrequency((float)minimalFreq);
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
      //AD9833setFrequency(freq, SINE);
      Ad9833.setFrequency((float)freq);
      delay(10);
    }

    ifreq = freqWithMaxI;

    //AD9833setFrequency(ifreq, SINE);
    Ad9833.setFrequency((float)ifreq);
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

 // lcd.begin(16,2,7);  // Зависит от версии библиотеки
  //lcd.begin();
  lcd.init();     // https://www.arduino.cc/reference/en/libraries/liquidcrystal-i2c/
  lcd.backlight();
  delay(10);
  ina219.begin(0x40); // (44) i2c address 64=0x40 68=0х44 исправлять и в ina219.h одновременно
  ina219.configure(0, 2, 12, 12, 7);      // 16S -8.51ms
  ina219.calibrate(0.100, 0.32, 16, 3.2);

  // This MUST be the first command after declaring the AD9833 object
  Ad9833.usesHWSPI();
  Ad9833.begin();              // The loaded defaults are 1000 Hz SINE_WAVE using REG0
  // The output is OFF, Sleep mode is disabled
//  Ad9833.EnableOutput(false);  // Turn OFF the output
  Ad9833.reset();
  // AD9833reset();            // Ресет после включения питания
  delay(10);

  // выставляем минимальную частоту для цикла определения максимального тока
  Ad9833.setWave(SINE); 
  Ad9833.setFrequency((float) FREQ_MIN);
//  Ad9833.EnableOutput(true);  // Turn OFF the output

  Serial.print("freq=");
  Serial.println(FREQ_MIN);

  // Настраиваем частоту под катушку
  readAnalogAndSetFreqInSetup();

  Data_ina219 = ina219.shuntCurrent() * 1000;
  myDisplay();
  delay(1000);
  PCICR |= (1 << PCIE2); // инициализируем порты для энкодера
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);
  startEncoder();

  memTimers = availableTimers[0]; // выставляем 15 минут по умолчанию

  wiperValue = 64;
  Potentiometer.writeValue(wiperValue); // Set MCP4131 to mid position
}

// *** ТЕЛО ПРОГРАММЫ ***
void loop() {
  mill = millis();
  Btn1.run();

  if (Btn1.read() == sbLong) {
    oldmemTimers = memTimers;
    timMillis = millis();
    isWorkStarted = 1;
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
  }

  readAnalogAndSetFreqInLoop();
}
