//
// Имя файла базы данных (не забудь поменять на своё)
//

// https://purecodecpp.com/archives/1502

struct myDB {
  int id = 1;
  char names[30] = "Abdominal inflammation";
  int f1 = 2720;
  int f2 = 2489;
  int f3 = 2170;
  int f4 = 2000;
  int f5 = 1865;
  int f6 = 1800;
  int f7 = 1600;
  int f8 = 1550;
  int f9 = 880;
  int f10 = 832;
  int f11 = 802;
  int f12 = 787;
  int f13 = 776;
  int f14 = 727;
  int f15 = 660;
  int f16 = 465;
  int f17 = 450;
  int f18 = 444;
  int f19 = 440;
  int f20 = 428;
  int f21 = 380;
  int f22 = 250;
  int f23 = 146;
  int f24 = 125;
  int f25 = 95;
  int f26 = 72;
  int f27 = 20;
  int f28 = 1;
  int f29 = 0;
  int f30 = 0;
  int f31 = NULL;
  int f32 = NULL;
  int f33 = NULL;
  int f34 = NULL;
  int f35 = NULL;
  int f36 = NULL;
  int f37 = NULL;
  int f38 = NULL;
  int f39 = NULL;
  int f40 = NULL;
  int f41 = NULL;
  int f42 = NULL;
};
//int zepFreq[42];

const char* DBName = "/spiffs/zepper.db";


//
// Простой запрос, показать таблицу Staff
//
const char* query = "SELECT * FROM frequency WHERE id = 51";

//
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
static int CallBack(void* tagregArray, int totalColumns, char** azColVals, char** azColNames) {
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

/*
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
  }
*/
    file = root.openNextFile();
    
    sqlite3* db;
    char* zErrMsg = 0;
    sqlite3_initialize();

    int rc = sqlite3_open(DBName /*"/spiffs/zepper.db"*/, &db);
    if (rc) {
      printf("Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return (1);
    }

    // Принимающий массив передаётся чертвёртым параметром!
    //int zepFreq[42];
    
    rc = sqlite3_exec(db, query, CallBack, zepFreq, &zErrMsg);
    if (rc != SQLITE_OK) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    else {
      // ПЕЧАТЬ массива для проверки
      for (int i = 0; i < sizeof(zepFreq) / sizeof(zepFreq[0]); i++) {
        printf("zepFreq[%d]=%d\n", i, zepFreq[i]);
      }
    }
    Serial.println(queryOne);
    
    sqlite3_close(db);
    return 0;
  }
