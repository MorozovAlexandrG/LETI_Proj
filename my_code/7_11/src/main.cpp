#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>

// --- ПИНЫ ---
#define AD_OUTPUT   2   // A0
#define PIN_BUTTON  9   // BOOT
#define SD_CS       20  // D7
#define SD_SCK      8   // D8
#define SD_MISO     7   // D5
#define SD_MOSI     10  // D10

// --- НАСТРОЙКИ WI-FI ---
const char* ssid = "PAKT-2985"; 
const char* password = "19439181";
const char* serverUrl = "http://httpbin.org/post"; // Тестовый сервер

bool isRecording = false;
File myFile;

void setup() {
  Serial.begin(115200);
  delay(3000); // Ждем открытия монитора
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  analogReadResolution(12);

  Serial.println("\n=== ИНИЦИАЛИЗАЦИЯ ===");
  
  // 1. Проверка SD
  Serial.print("1. SD-карта: ");
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ НЕ НАЙДЕНА!");
  } else {
    Serial.println("✅ OK");
  }

  // 2. Подключение к Wi-Fi
  Serial.print("2. Wi-Fi: ");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" ✅ ПОДКЛЮЧЕН!");
  } else {
    Serial.println(" ❌ ОШИБКА (работаем без сети)");
  }

  Serial.println("=================================");
  Serial.println("Коротко нажми = СТАРТ/СТОП записи (Сырые данные в порт)");
  Serial.println("Зажми на 1 сек = ВЫГРУЗКА файла (Для Питона) + Отправка");
  Serial.println("=================================");
}

void loop() {
  // --- ЛОГИКА КНОПКИ ---
  if (digitalRead(PIN_BUTTON) == LOW) {
    unsigned long pressStart = millis();
    
    // Ждем, пока кнопку держат
    while (digitalRead(PIN_BUTTON) == LOW) {
      delay(10);
      
      // ДОЛГОЕ НАЖАТИЕ (ВЫГРУЗКА)
      if (millis() - pressStart > 1000) {
        Serial.println("\n[ДЕЙСТВИЕ]: Долгое нажатие. Останавливаем запись и выгружаем.");
        isRecording = false;
        if (myFile) myFile.close();
        
        // 1. Чтение с SD (для Питона и для сервера)
        Serial.println("[ШАГ 3]: Чтение данных с SD-карты...");
        File readFile = SD.open("/ecg_data.csv");
        String fileContent = "";
        
        if (readFile) {
          Serial.println("--- НАЧАЛО ВЫГРУЗКИ ---");
          while (readFile.available()) {
            String line = readFile.readStringUntil('\n');
            if (line.length() > 0) { // Защита от пустых строк
              Serial.println(line); 
              fileContent += line + "\n";
            }
          }
          readFile.close();
          Serial.println("--- КОНЕЦ ВЫГРУЗКИ ---");
        } else {
          Serial.println("Файл не найден!");
        }
        
        // 2. Отправка на сервер
        Serial.println("[ШАГ 4]: Отправка данных на сервер...");
        if (WiFi.status() == WL_CONNECTED) {
          HTTPClient http;
          http.begin(serverUrl);
          http.addHeader("Content-Type", "text/csv");
          int code = http.POST(fileContent);
          
          if (code > 0) {
            Serial.print("✅ Ответ сервера: ");
            Serial.println(code);
          } else {
            Serial.print("❌ Ошибка сервера: ");
            Serial.println(code);
          }
          http.end();
        } else {
          Serial.println("❌ Нет Wi-Fi для отправки.");
        }
        
        while (digitalRead(PIN_BUTTON) == LOW) delay(10);
        return; 
      }
    }
    
    // КОРОТКОЕ НАЖАТИЕ (СТАРТ/СТОП)
    isRecording = !isRecording;
    if (isRecording) {
      Serial.println("\n[ДЕЙСТВИЕ]: Старт записи.");
      
      bool fileExists = SD.exists("/ecg_data.csv");
      myFile = SD.open("/ecg_data.csv", FILE_APPEND);
      
      if (myFile) {
        if (!fileExists) myFile.println("Time_ms,Value");
      } else {
        Serial.println("Ошибка открытия файла!");
      }
    } else {
      Serial.println("\n[ДЕЙСТВИЕ]: Стоп записи.");
      if (myFile) myFile.close();
    }
    delay(300);
  }

  // --- ЗАПИСЬ ЭКГ И СЫРЫЕ ДАННЫЕ В ПОРТ ---
  if (isRecording) {
    int val = analogRead(AD_OUTPUT);
    if (myFile) {
      myFile.print(millis());
      myFile.print(",");
      myFile.println(val);
    }
    Serial.println(val); // ВОТ ОНИ, СЫРЫЕ ДАННЫЕ ДЛЯ РЕАЛЬНОГО ВРЕМЕНИ
    delay(4); 
  }
}