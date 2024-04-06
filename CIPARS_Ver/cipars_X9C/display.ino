void myDisplay() {
  // 1 строка
  lcd.setCursor(0, 0);
  if (!isWorkStarted) {

#ifdef LCD_RUS
    lcd.print("Время-");
    lcd.print(memTimers / 60000);
    lcd.print(" мин. ");
#else
    lcd.print("Times-");
    lcd.print(memTimers / 60000);
    lcd.print(" min. ");
#endif
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
    lcd.print(currentPotenciometrPercent);
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

 
