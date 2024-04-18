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

int readSQLite3() {
  printf("Go on!!!\n\n");

  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("Failed to mount file system");
    return 0;
  }

  // list SPIFFS contents
  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("- failed to open directory");
    return 0;
  }

  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return 0;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }

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
    for (int i = 0; i < sizeof(zepDbFreq) / sizeof(zepDbFreq[0]); i++) {
      printf("zepDbFreq[%d]=%d\n", i, zepDbFreq[i]);
    }
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
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbExpo) / sizeof(zepDbExpo[0]); i++) {
      printf("zepDbExpo[%d]=%d\n", i, zepDbExpo[i]);
    }
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
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbPause) / sizeof(zepDbPause[0]); i++) {
      printf("zepDbPause[%d]=%d\n", i, zepDbPause[i]);
    }
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
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbModeGen) / sizeof(zepDbModeGen[0]); i++) {
      printf("zepDbModeGen[%d]=%d\n", i, zepDbModeGen[i]);
    }
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
    // ПЕЧАТЬ массива для проверки
    for (int i = 0; i < sizeof(zepDbModeSig) / sizeof(zepDbModeSig[0]); i++) {
      printf("zepDbModeSig[%d]=%d\n", i, zepDbModeSig[i]);
    }
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
  
  return 0;
}    // ************** END SQLITE3 *************** //
