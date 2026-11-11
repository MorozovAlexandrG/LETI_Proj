#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#define AD_OUTPUT 2
#define SD_CS 6
#define PIN_BUTTON 9

File dataFile;
bool isRecording = false;
int fileIndex = 1;
bool lastButtonState = HIGH;
unsigned long lastDebounce = 0;
bool sdReady = false;
unsigned long lastTry = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  analogReadResolution(12);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Serial.println("=============================");
  Serial.println("=== СТАРТ СИСТЕМЫ ===");
  Serial.println("=============================");
  
  SPI.begin(8, 5, 10, 6);
  
  // Пробуем инициализировать SD
  if (SD.begin(SD_CS)) {
    sdReady = true;
    Serial.println("✅ SD карта готова!");
  } else {
    Serial.println("❌ SD карта НЕ найдена. Буду пробовать каждые 3 сек...");
  }
  
  Serial.println("Система запущена. Жду кнопку или SD...");
  Serial.println("=============================");
}

void loop() {
  // Если SD не готова — пробуем снова каждые 3 секунды
  if (!sdReady && millis() - lastTry > 3000) {
    lastTry = millis();
    Serial.println("Попытка подключить SD...");
    if (SD.begin(SD_CS)) {
      sdReady = true;
      Serial.println("✅ SD карта подключилась!");
    }
  }

  // Кнопка
  bool buttonState = digitalRead(PIN_BUTTON);
  
  if (buttonState == LOW && lastButtonState == HIGH && (millis() - lastDebounce > 300)) {
    lastDebounce = millis();
    
    if (!sdReady) {
      Serial.println("⚠️ Сначала дождись подключения SD карты!");
    } else if (!isRecording) {
      String fileName = "/ecg_" + String(fileIndex) + ".csv";
      dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        dataFile.println("Time_ms,ECG_Value");
        isRecording = true;
        Serial.println(">>> START: " + fileName);
        fileIndex++;
      }
    } else {
      dataFile.close();
      isRecording = false;
      Serial.println(">>> STOP. Сохранено.");
    }
  }
  lastButtonState = buttonState;

  // Запись данных
  if (isRecording && dataFile && sdReady) {
    int ecgValue = analogRead(AD_OUTPUT);
    dataFile.print(millis());
    dataFile.print(",");
    dataFile.println(ecgValue);
    dataFile.flush();
    Serial.println(ecgValue);
  }
  
  delay(4);
}