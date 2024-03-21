// Генератор для катушки Мишина на основе DDS AD9833
// (c)UA6EM
/*
 * Версия от 12.06.2019 - Добавлено измерение напряжения, вывод на дисплей, установка
 * начальной частоты определённой в дефайне, если не найден резонанс катушки
 * - напряжение показывало отрицательное, ina219 был сконфигурирован на адрес 44, 
 * - поправил скетч пробовал в работе при запитке от USB, на Vin+ подавал +5V с ардуины
 * - вывод Vin- (ina219) нагрузил резистором 10 ком, напряжение показывает, ток - 0 )))
 *  
 *  - Версия от 22.06.2019 - Добавлен зуммер, работает через прерывание 1, 
 *                           формируется с частотой 490 герц
 * - добавлена библиотека модуля DAC для регулировки усиления, выставляет первичное
 *   значение на выходе модуля в 0.8 вольта (регулировка усиления TDA-7056) 
 * - Версия от 04.08.2019 - Добавлена процедура рассчета скользящего среднего (moving average)
 * -
 */
#define SECONDS(x) ((x) * 1000UL)
#define MINUTES(x)  (SECONDS(x) * 60UL)
#define HOURS(x)  (MINUTES(x) * 60UL)
#define DAYS(x)   (HOURS(x) * 24UL)
#define WEEKS(x)  (DAYS(x) * 7UL)
unsigned long interval = MINUTES(1);

#include <Wire.h>
#include <SPI.h>
#include "LiquidCrystal_I2C.h"        // брал здесь - https://iarduino.ru/file/134.html
LiquidCrystal_I2C lcd(0x3F, 20, 4);   // Для экрана 20х4, I2C адрес дисплея уточнить  
//#include <Adafruit_INA219.h>
//Adafruit_INA219 ina219;

#include "INA219.h"
INA219 ina219;

//#define TWBR  //Зарезервировано для частоты обмена с DAC в 400Кгц
#include <Adafruit_MCP4725.h>
Adafruit_MCP4725 dac;
unsigned int dVolume = 737; // Напряжение на выходе DAC 
                            // около 0.9 вольта

#define PIN_ZUM 12  //  был 10 пин
#define pinINT1 3   // этот пин Шимим 490Гц 
#define zFreq 2     // делитель интервала - секунда/2

unsigned int imax = 0;
unsigned int Data_ina219 = 0;
volatile float Voltage_ina219 = 0;
 
const int SINE = 0x2000;                    // определяем значение регистров AD9833 взависимости от формы сигнала
const int SQUARE = 0x2020;                  // После обновления частоты нужно определить форму сигнала
const int TRIANGLE = 0x2002;                // и произвести запись в регистр.
 
const float refFreq = 25000000.0;           // Частота кристалла на плате AD9833

#define Fdefine 300000
long Fmin = 50000;
long Fmax = 500000;
#define Ftune 10000
//int Ftune = 10000;
unsigned int di = (Fmax-Fmin)/Ftune -1;
long FFmax = 0;
long freq = Fmin;
long ifreq = Fdefine; // если не будет определена частота резонанса катушки, 
                      // то она установится в это значение
                      
//int Ffinetune = 200;
#define  Ffinetune 200
  
/*const int FSYNC = 10;                       // Standard SPI pins for the AD9833 waveform generator.
const int CLK = 13;                         // CLK and DATA pins are shared with the TFT display.
const int DATA = 11; 
*/
#define FSYNC 10
#define CLK 13
#define DATA 11
#define KnobEncoder 5         // кнопка энкодера

 byte myClock = 0;
 byte SetClock = 5;
 byte flagClock = 0;
 byte myBuzzer = 0;
 unsigned long oldMillis = 0;

/*
const char toks[]PROGMEM  = "I = ";
const char freqs[]PROGMEM  = "Freq = ";
const char call[]PROGMEM  ="Generator AD9833";
*/
 
 /********* используемые подпрограммы выносим сюда *********/
 
/*******************ПИЩАЛКА ********************/
void start_Buzzer(){
     pinMode(PIN_ZUM,OUTPUT);
     attachInterrupt(1, Buzzer, RISING );
     analogWrite(pinINT1,0x80); //установим на пине частоту 
                                //490 гц скважность 2
 }

void stop_Buzzer(){
     detachInterrupt(1);
     digitalWrite(PIN_ZUM,LOW);
 }

void Buzzer(void){
     static int i=490/zFreq;
     if(!i--)
     {
    digitalWrite(PIN_ZUM, ! digitalRead(PIN_ZUM));
    i=490/zFreq;
      }
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
 digitalWrite(FSYNC, LOW);           // Set FSYNC low before writing to AD9833 registers
 delayMicroseconds(10);              // Give AD9833 time to get ready to receive data.
 SPI.transfer(highByte(dat));        // Each AD9833 register is 32 bits wide and each 16
 SPI.transfer(lowByte(dat));         // bits has to be transferred as 2 x 8-bit bytes.
  digitalWrite(FSYNC, HIGH);          //Write done. Set FSYNC high
}

//**** Процедура грубой настройки частоты по максимальному току ***/
 void setFreq(){
 for (unsigned int i=1; i <= di; i++) {
   // Data_ina219=ina219.getCurrent_mA();
      Data_ina219=ina219.shuntCurrent() * 1000; 
    if (Data_ina219 > imax){ imax=Data_ina219; ifreq = freq; } // Если значение больше, то запомнить
    if (freq >=Fmax) {freq = Fmax;}
     freq=freq+Ftune;
     AD9833setFrequency(freq, SINE);
     delay(20);
    } 
 }

//*** Процедура тонкой настройки частоты по максимальному току ***/
 void setFFreq(){
    for (int j=1; j <= 100; j++) {
     //Data_ina219=ina219.getCurrent_mA();
     Data_ina219=ina219.shuntCurrent() * 1000; 
     if (Data_ina219 > imax){ imax=Data_ina219; ifreq = freq; } // Если значение больше, то запомнить
     freq=freq+Ffinetune;
     if (freq >=FFmax) {freq = FFmax;} 
     AD9833setFrequency(freq, SINE);
     delay(20);
     //Data_ina219=ina219.getCurrent_mA();
     //Data_ina219=ina219.shuntCurrent() * 1000; 
    } 
 }

 void myDisplay(){
   lcd.setCursor(2, 0);                  // 1 строка
   lcd.print("Freq = ");
   lcd.setCursor(9, 0);                   //1 строка 7 позиция
   float freq_tic = ifreq/1000;
   lcd.print(freq_tic,2);
   lcd.print("kHz");
   lcd.setCursor(2, 1);                  // 2 строка
   lcd.print("I = ");
// lcd.setCursor(7, 1);                  // 2 строка 7 позиция
   lcd.print(Data_ina219);
   lcd.print("ma");
   lcd.print("    ");  // затираем хвост при смене числа значащих значений
   lcd.setCursor(2, 2); 
   lcd.print("Vpp = ");
   lcd.print(Voltage_ina219,2);
   lcd.print("V");
   lcd.setCursor(2, 3); 
   lcd.print("Generator AD9833");
 }

/****************** MOVING AVERAGE *************************/
//Процедура рассчета скользящего среднего

void smooth(double *input, double *output, int n, int window)
{
   int i,j,z,k1,k2,hw;
   double tmp;
   if(fmod(window,2)==0) window++;
   hw=(window-1)/2;
   output[0]=input[0];

   for (i=1;i<n;i++){
       tmp=0;
       if(i<hw){
           k1=0;
           k2=2*i;
           z=k2+1;
       }
       else if((i+hw)>(n-1)){
           k1=i-n+i+1;
           k2=n-1;
           z=k2-k1+1;
       }
       else{
           k1=i-hw;
           k2=i+hw;
           z=window;
       }

       for (j=k1;j<=k2;j++){
           tmp=tmp+input[j];
       }
       output[i]=tmp/z;
   }
} //end-function


//************************** SETUP *************************/
void setup() { 
  SPI.begin();
  Serial.begin(115200);
  pinMode(PIN_ZUM, OUTPUT);
   pinMode(5, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
     pinMode(7, INPUT_PULLUP);
  
  dac.begin(0x62);                // I2C адрес MCP4725 (может работать на 
                                  // частоте 400 кгц
  dac.setVoltage(dVolume, true);  // выставим регулятор усиления в 0.8 вольт                                                                                  dac.setVoltage(dVolume, true);  // установить напряжение 0.8 вольт
                                  // с запоминанием во флэше DAC
                                  // будет выставлять при подаче питания
  
 // lcd.init(); //для библиотеки V112
  lcd.begin();
  lcd.backlight();
  delay(10);   
    
 // ina219.begin(0x40); //такая конфигурация конфликтует с дисплеем
 // ina219.begin();
 // ina219.setCalibration_16V_400mA(); 
 // ina219.setCalibration_32V_2A();  // Интересно, в библиотеку можно внести изменения
  delay(10);                       // сделав свои параметры калибровки? надо 16V 2A

  ina219.begin(0x44); // (44) i2c address 64=0x40 68=0х44 исправлять и в ina219.h одновременно
  ina219.configure(0, 2, 12, 12, 7); // 16S -8.51ms
 // monitor.configure(0, 2, 10, 10, 7); // 4S -2.13ms
 // monitor.configure(0, 2, 11, 11, 7); // 8S -4.26ms
 // monitor.configure(0, 2, 12, 12, 7); // 16S -8.51ms
 // monitor.configure(0, 2, 13, 13, 7); // 32S -17.02ms
 // monitor.configure(0, 2, 14, 14, 7); // 64S -34.05ms
 // monitor.configure(0, 2, 15, 15, 7);  // 128S - 68.10ms
 // monitor.configure(0, 2, 8, 8, 7);
                           // range, gain, bus_adc, shunt_adc, mode
                           // range = 1 (0-32V bus voltage range)
                           // gain = 3 (1/8 gain - 320mV range)
                           // bus adc = 3 (12-bit, single sample, 532uS conversion time)
                           // shunt adc = 3 (12-bit, single sample, 532uS conversion time)
                           // mode = 7 (continuous conversion)

  ina219.calibrate(0.100, 0.32, 16, 3.2); 
                // R_шунта, напряж_шунта, макcнапряж, максток
 
  
  AD9833reset();                   // Ресет после включения питания
  delay(10);
  AD9833setFrequency(freq, SINE);  // выставляем нижнюю частоту
  //
  setFreq();
  Serial.print("freq=");
  Serial.println(freq);
  freq = ifreq-10000;
  FFmax =ifreq +10000;
  imax = 0;
  AD9833setFrequency(freq, SINE); 
  setFFreq();
  Serial.print("ffreq=");
  Serial.println(ifreq);
  AD9833setFrequency(ifreq, SINE); // выставляем частоту максимального тока
 // Data_ina219=ina219.getCurrent_mA();
  Data_ina219=ina219.shuntCurrent() * 1000; 
  Voltage_ina219 = ina219.busVoltage();
  myDisplay();
  delay(1);

  }    // Конец процедуры инициализации прибора
  
 
// *** ТЕЛО ПРОГРАММЫ ***
 
void loop() {
  if(millis()-oldMillis >= interval && flagClock == 1){
  myClock++;  //Подсчитываем прошедшие минуты
  oldMillis=millis();
 }
 if(myClock == SetClock){
  start_Buzzer();
  flagClock = 0;
  myClock = 0;
  myBuzzer = 1;
 }
 if(myBuzzer==1 && !digitalRead(KnobEncoder)){
  stop_Buzzer();
  myBuzzer=0;
 }
 
 // Data_ina219=ina219.getCurrent_mA();
  Data_ina219=ina219.shuntCurrent() * 1000; 
  Voltage_ina219 = ina219.busVoltage();
  myDisplay();

   /*
   Serial.println();
   Serial.print("Напряжение = ");
   Serial.println( Voltage_ina219,2);
   Serial.print("Ток = ");
   Serial.println(Data_ina219);
   Serial.println();
   */
   if(digitalRead(6)==LOW){
    dVolume = dVolume - 5;
     dac.setVoltage(dVolume, true);
      delay(200);
   }
   if(digitalRead(7)==LOW ||digitalRead(5)==LOW){
    dVolume = dVolume + 5;
     dac.setVoltage(dVolume, true);
      delay(200);
   }
   
   delay(200);
 } //END
