/*
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
int zepFreq[42];

// Обработчик функции возврата данных из базы
const char *tagregArray = "Callback function called";
/*
static int CallBack(void *datas, int argc, char **argv, char **azColName) {
  int i = 0;
  Serial.printf("%s: ", (const char *)datas);
  int s = atoi((argv[i]));
  if (s == 51) {
    for (i = 2; i < argc; i++) {
      zepFreq[i - 2] = atoi((argv[i]));
      //Serial.printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    for (int j = 0; j < argc; j++) {
      Serial.print("f");
      Serial.print(j + 1);
      Serial.print(" = ");
      Serial.println(zepFreq[j]);
      yield();
    }
    Serial.printf("\n");
    return 0;
  }
}
*


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


// Открытие базы данных
int dBopen(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}

// Функция запроса к базе данных
char *gzErrMsg = 0;
int dBexec(sqlite3 *db, const char *sql) {
  Serial.println(sql);
  long start = micros();
  int rc = sqlite3_exec(db, sql, CallBack, (void *)tagregArray, &gzErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", gzErrMsg);
    sqlite3_free(gzErrMsg);
  } else {
    Serial.printf("Operation done successfully\n");
  }
  Serial.print(F("Time taken:"));
  Serial.println(micros() - start);
  return rc;
}

// Функция работы с базой данных (извлечение параметров настройки генератора)
void readSQLite3() {
  sqlite3 *db1;
  int rc;
  
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("Failed to mount file system");
    return;
  }

  // list SPIFFS contents
  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }
  yield();
  File file = root.openNextFile();
  yield();
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
  yield();
  sqlite3_initialize();
  if (db_open("/spiffs/zepper.db", &db1))
    return;

  yield();
  int nr = 51;
  const  char * sql;
 /*
  sql = "SELECT * FROM frequency WHERE id =";
  sql+= String(nr);
  sql+= "'\"";
  
  rc = dBexec(db1, sql = "SELECT count(*) FROM frequency");
  //SELECT id, COUNT(*) FROM times WHERE status = TRUE GROUP BY id ORDER BY COUNT(*) DESC
*
  Serial.print("COUNT = ");
  //Serial.println(cnt); // не правильно
  Serial.println();

  rc = dBexec(db1, "SELECT * FROM frequency WHERE id = 51");
  if (rc != SQLITE_OK) {
    sqlite3_close(db1);
    return;
  }
  yield();
  sqlite3_close(db1);
}

*/
