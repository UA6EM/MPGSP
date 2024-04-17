void goZepper() {
  int sst = 0;   // возьмём нулевой элемент массива
  bool fgen = false; // синус / меандр = true (отслеживание смены режима)

  do {
    printStruct();

    if (Cicle.Freq > 0) {                      // Если в массиве частота не 0 то работаем по циклограмме
      // Режим генератора синус/меандр
      if (fgen != Cicle.ModeGen) {             // смена режима, пауза, зумм 5 секунд
        fgen = Cicle.ModeGen;
        Serial.println("Смена режима выхода");
        start_Buzzer();                        // Звуковой сигнал
        testTFT(10 * 1000);
        //delay(5000);
        Serial.println("Конец паузы 10 секунд");
        stop_Buzzer();
      }

      if (!fgen) {                              // *** Режим синуса ***
        digitalWrite(PIN_RELE, LOW);            // Выход реле (NC)
        int power = 20;                         // Очки, половинная мощность (5 вольт) - 20%
        setResistance(map(power, 0, 100, 0, d_resis));
        Serial.print("U = ");
        Serial.print(power);
        Serial.println(" %");

        digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода
        Ad9833.setFrequency(Cicle.Freq, Cicle.ModeSig);
        Serial.print("Частота ");
        Serial.print((float)Cicle.Freq / 1000,3);
        Serial.println(" KHz");
        readDamp(map(power, 0, 100, 0, d_resis));    // Получить позицию энкодера

        lcd.setCursor(0, 0);
        lcd.print("  F - ");
        lcd.print((float)Cicle.Freq / 1000,3);
        lcd.print(" KHz  ");
        lcd.setCursor(0, 1);
        lcd.print("ЖдёM ");
        lcd.print(Cicle.Exposite / 60);
        lcd.print("-е минуты");
        testTFT(Cicle.Exposite * 1000);
        //delay(Cicle.Exposite * 1000);           // Выдержка экспозиции частоты

        if (Cicle.Pause != 0) {                 // Отработаем паузу, если она есть
          Serial.print("Пауза ");
          Serial.print(Cicle.Pause);
          Serial.println(" секунд");
          digitalWrite(ON_OFF_CASCADE_PIN,LOW); // Разрешение выхода
          testTFT(Cicle.Pause * 1000);
          //delay(Cicle.Pause * 1000);
          digitalWrite(ON_OFF_CASCADE_PIN,HIGH);// Разрешение выхода
        }
      } else {                                  // *** Режим меандра ***
        digitalWrite(PIN_RELE, HIGH);            // Выход реле (NO)
        int power = 50;                          // ZEPPER, полная мощность (12 вольт) - УТОЧНИТЬ!!!
        setResistance(map(power, 0, 100, 0, d_resis));
        Serial.print("U = ");
        Serial.print(power);
        Serial.println(" %");
        
        digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода
        Ad9833.setFrequency(Cicle.Freq, Cicle.ModeSig);
        Serial.print("Частота ");
        Serial.print((float)Cicle.Freq / 1000,3);
        Serial.println(" KHz");
        readDamp(map(power, 0, 100, 0, d_resis));    // Получить позицию энкодера

        lcd.setCursor(0, 0);
        lcd.print("  F - ");
        lcd.print((float)Cicle.Freq / 1000,3);
        lcd.print(" KHz  ");
        lcd.setCursor(0, 1);
        lcd.print("ЖдёM ");
        lcd.print(Cicle.Exposite / 60);
        lcd.print("-е минуты");
        testTFT(Cicle.Exposite * 1000);
       // delay(Cicle.Exposite * 1000);           // Выдержка экспозиции частоты

        if (Cicle.Pause != 0) {                 // Отработаем паузу, если она есть
          Serial.print("Пауза ");
          Serial.print(Cicle.Pause);
          Serial.println(" секунд");
          digitalWrite(ON_OFF_CASCADE_PIN,LOW); // Разрешение выхода
          testTFT(Cicle.Pause * 1000);
         // delay(Cicle.Pause * 1000);
          digitalWrite(ON_OFF_CASCADE_PIN,HIGH);// Разрешение выхода
          }
      }
    sst += 1;

    Cicle.ModeGen  = zepDbModeGen[sst];
    Cicle.ModeSig  = zepDbModeSig[sst];
    Cicle.Freq     = zepDbFreq[sst];
    Cicle.Exposite = zepDbExpo[sst];
    Cicle.Pause    = zepDbPause[sst];
    }
  } while (Cicle.Freq > 0 );
   
    printStruct();
    Serial.println("Сеанс окончен");
    digitalWrite(ON_OFF_CASCADE_PIN,LOW); // Разрешение выхода OFF
    digitalWrite(PIN_RELE, LOW);            // Выход реле (NC)
    sst = 0; 
    
    // В структуру восстановить первую запись   
    Cicle.ModeGen  = zepDbModeGen[sst];
    Cicle.ModeSig  = zepDbModeSig[sst];
    Cicle.Freq     = zepDbFreq[sst];
    Cicle.Exposite = zepDbExpo[sst];
    Cicle.Pause    = zepDbPause[sst];

    isWorkStarted = false; // рабочий режим окончен
}


  /*
  for (int i = 0; i < 42; i++) {
    Serial.print("f");
    Serial.print(i + 1);
    Serial.print(" = ");
    Serial.println(zepDbFreq[i]);
  }

  Serial.println(queryFreq);
  */
  /*
    Cicle.ModeGen  = zepDbModeGen[sstruc];
    Cicle.ModeSig  = zepDbModeSig[sstruc];
    Cicle.Freq     = zepDbFreq[sstruc];
    Cicle.Exposite = zepDbExpo[sstruc];
    Cicle.Pause    = zepDbPause[sstruc];

    struct  {
    int ModeGen;   // 0 - GENERATOR 1 - ZEPPER
    int ModeSig;   // 0 - OFF, 1 - SINE, 2 - QUADRE1, 3 - QUADRE2, 4 - TRIANGLE
    int Freq;      // Частота сигнала
    int Exposite;  // Время экспозиции
    int Pause;     // Пауза после отработки времени экспозиции
    } Cicle;
  */
