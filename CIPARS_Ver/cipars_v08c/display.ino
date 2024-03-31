void myDisplay() {
  // 1 строка
  lcd.setCursor(0, 0);
  if (!isWorkStarted) {
#ifdef UA6EM
    lcd.print("Время-");
#else
    lcd.print("Times-");
#endif
    lcd.print(memTimers / 60000);
    if (memTimers / 60000 > 0) {
#ifdef UA6EM
      lcd.print(" мин. ");
#else
      lcd.print(" min. ");
#endif
    } else {
#ifdef UA6EM
      lcd.print("0 мин. ");
#else
      lcd.print("0 min. ");
#endif
    }
  } else {
    lcd.print("Т-");
    if (memTimers > 60000) {
      // если больше минуты, то показываем минуты
      lcd.print(memTimers / 1000 / 60);
#ifdef UA6EM
      lcd.print("мин.");
#else
      lcd.print("min.");
#endif
    } else {
      // если меньше минуты, то показываем секунды
      lcd.print(memTimers / 1000);
#ifdef UA6EM
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
