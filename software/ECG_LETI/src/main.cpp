#include <Arduino.h>

#define BUTTON_PIN 2  // D0 на XIAO ESP32C3

// Параметры антидребезга и фильтрации
const uint16_t DEBOUNCE_MS   = 50;   // время стабильного состояния для подтверждения
const uint16_t SAMPLE_MS     = 5;    // период опроса пина

// Состояние
bool lastStableState = HIGH;         // подтверждённое состояние (HIGH = не нажат)
bool lastRawState    = HIGH;         // последнее сырое чтение
uint32_t lastChangeTime = 0;         // момент последнего изменения сырого сигнала
uint32_t pressCount = 0;             // счётчик нажатий

void setup() {
    Serial.begin(115200);

    // Внешняя подтяжка к VCC, кнопка/провод замыкает на GND
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Включаем встроенную подтяжку к 3.3V


    lastRawState = digitalRead(BUTTON_PIN);
    lastStableState = lastRawState;

    Serial.println("Готов. Считаем замыкания D0 на GND...");
}

void loop() {
    bool raw = digitalRead(BUTTON_PIN);

    // Если сырое состояние изменилось — сбрасываем таймер стабильности
    if (raw != lastRawState) {
        lastRawState = raw;
        lastChangeTime = millis();
    }

    // Если сигнал держится стабильно дольше DEBOUNCE_MS — фиксируем переход
    if ((millis() - lastChangeTime) >= DEBOUNCE_MS && raw != lastStableState) {
        lastStableState = raw;

        // Переход HIGH -> LOW = замыкание (нажатие)
        if (lastStableState == LOW) {
            pressCount++;
            Serial.print("Кнопка нажата. Счётчик: ");
            Serial.println(pressCount);
        }
        // Переход LOW -> HIGH (размыкание) намеренно игнорируем
    }

    delay(SAMPLE_MS);
}