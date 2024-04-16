void goZepper() {
  int sst = 0;   // возьмём нулевой элемент массива
  bool fgen = false; // синус / меандр = true (отслеживание смены режима)
  /*
    Cicle.ModeGen  = zepDbModeGen[sstruc];
    Cicle.ModeSig  = zepDbModeSig[sstruc];
    Cicle.Freq     = zepDbFreq[sstruc];
    Cicle.Exposite = zepDbExpo[sstruc];
    Cicle.Pause    = zepDbPause[sstruc];
  */
  do {

    if ((long)Cicle.Freq > 0) {          // Если в массиве частота не 0 то работаем по циклограмме
        // Режим генератора синус/меандр
        if (fgen != Cicle.ModeSig) {       // смена режима, пауза, зумм 5 секунд
          fgen = Cicle.ModeSig;
          start_Buzzer();                  // Звуковой сигнал
          delay(5000);
          stop_Buzzer();
        }

        if (!fgen) {                              // *** Режим синуса ***
          digitalWrite(PIN_RELE, LOW);            // Выход реле (NC)
          int power = 5;                          // Очки, половинная мощность (5 вольт)
          setResistance(map(power, 0, 12, 0, 100));
          Serial.print("U = ");
          Serial.println(map(power, 0, 12, 0, 100));

          digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода
          Ad9833.setFrequency(Cicle.Freq, AD9833_SINE);
          Serial.print("Частота ");
          Serial.print(Cicle.Freq / 1000);
          Serial.println(" KHz");
          readDamp(map(power, 0, 12, 0, 100));    // Получить позицию энкодера

          lcd.setCursor(0, 0);
          lcd.print("  F - ");
          lcd.print(Cicle.Freq / 1000);
          lcd.print(" KHz  ");
          lcd.setCursor(0, 1);
          lcd.print("ЖдёM ");
          lcd.print(Cicle.Exposite / 60);
          lcd.print("-е минуты");
          delay(Cicle.Exposite * 1000);           // Выдержка экспозиции частоты

          if (Cicle.Pause != 0) {                 // Отработаем паузу, если она есть
            Serial.print("Пауза ");
            Serial.print(Cicle.Pause);
            Serial.print(" секунд");
            digitalWrite(ON_OFF_CASCADE_PIN, LOW); // Разрешение выхода
            delay(Cicle.Pause * 1000);
            digitalWrite(ON_OFF_CASCADE_PIN, HIGH); // Разрешение выхода
          }
        } else {                                  // *** Режим меандра ***

        }
      }
      sst+= 1;

      Cicle.ModeGen  = zepDbModeGen[sst];
      Cicle.ModeSig  = zepDbModeSig[sst];
      Cicle.Freq     = zepDbFreq[sst];
      Cicle.Exposite = zepDbExpo[sst];
      Cicle.Pause    = zepDbPause[sst];

    } while(Cicle.Freq > 0 );
}
