#include <Arduino.h>

#define AD_OUTPUT A0
#define PIN_BUTTON 9 // Кнопка B (Boot) на самой плате

bool isRecording = false;
int lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Настраиваем встроенную кнопку B
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  
  analogReadResolution(12);
  Serial.println("Система готова. Нажми кнопку B для старта.");
}

void loop() {
  int reading = digitalRead(PIN_BUTTON);

  // Ловим нажатие (переход из HIGH в LOW)
  if (reading == LOW && lastButtonState == HIGH) {
    isRecording = !isRecording;
    
    if (isRecording) {
      Serial.println("--- СТАРТ ---");
    } else {
      Serial.println("--- СТОП ---");
    }
    
    delay(50); // Антидребезг
  }
  
  lastButtonState = reading;

  if (isRecording) {
    int val = analogRead(AD_OUTPUT);
    Serial.println(val);
    delay(4); // 250 Гц
  }
}