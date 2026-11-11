#include <Arduino.h>
#include "config.h"
#include "ButtonManager.h"
#include "ECG_Recorder.h"
#include "NetworkManager.h"

WireButton button(PIN_BUTTON);
ECG_Recorder ecg;
NetworkManager network;

void setup() {
    Serial.begin(115200);
    delay(1000); // Ждем инициализации USB
    Serial.println("=== ECG Project Started ===");

    button.begin();
    ecg.begin();
    network.begin();
    
    // Пробуем подключиться к WiFi при старте
    Serial.println("Connecting to WiFi...");
    network.connectToSavedWiFi();
}

void loop() {
    // 1. Опрос кнопки
    button.update();

    // Долгое нажатие (2000 мс) переключает режим записи
    if (button.isLongPressed(2000)) {
        if (ecg.getState() == STATE_OFF) {
            Serial.println(">>> START RECORDING");
            ecg.triggerStart();
        } else {
            Serial.println(">>> STOP RECORDING");
            ecg.triggerStop();
            
            // Если WiFi подключен, можно отправить данные (заглушка на будущее)
            if (network.isConnectedToHomeWiFi()) {
                Serial.println("WiFi connected. Ready to send data to server.");
                // network.uploadLastFile(); // Раскомментируем, когда сделаем сервер
            }
        }
    }

    // 2. Обновление рекордера (сбор данных и запись на SD в фоне)
    ecg.update();

    // 3. Обработка сетевых событий (если вдруг включится режим настройки)
    network.handleClient();
}