#include <Arduino.h>

// Назначение пинов XIAO ESP32-C3
#define AD_OUTPUT  2 // Аналоговый выход AD8232
#define LO_MINUS   4 // Индикатор отсоединения LO-
#define LO_PLUS    3 // Индикатор отсоединения LO+

void setup() {
  // Инициализация Serial для плоттера
  Serial.begin(115200);
  delay(1000); // Небольшая задержка для инициализации USB CDC на ESP32-C3

  // LO+ и LO- работают как цифровые входы
  pinMode(LO_MINUS, INPUT);
  pinMode(LO_PLUS, INPUT);

  // Настройка АЦП (опционально, по умолчанию 12 бит)
  analogReadResolution(12); 
}

void loop() {
  // Проверяем состояние электродов
  // Если хотя бы один электрод отсоединен, LO+ или LO- станут HIGH
  bool leadsOff = digitalRead(LO_MINUS) || digitalRead(LO_PLUS);

  if (leadsOff) {
    // Если электроды отпали, рисуем 0 на графике
    Serial.println(0);
  } else {
    // Считываем аналоговый сигнал ЭКГ
    int ecgValue = analogRead(AD_OUTPUT);
    Serial.println(ecgValue);
  }

  // Задержка для корректного отображения в плоттере 
  // (20 мс дает частоту обновления 50 Гц, этого достаточно для ЭКГ)
  delay(20); 
}