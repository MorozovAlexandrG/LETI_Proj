#include <Arduino.h>

#include "button.h"
#include "heart.h"
#include "DisplayManager.h"

#define BUTTON_PIN 2   // D0 на XIAO ESP32-C3

Button myButton(BUTTON_PIN);

// --- Тайминги задач ---
static const uint32_t UI_PERIOD_MS  = 100;  // 10 Гц — отрисовка OLED
static const uint32_t ECG_PERIOD_MS = 20;    // 250 Гц — сэмплирование ЭКГ

static uint32_t lastUiTime  = 0;
static uint32_t lastEcgTime = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    myButton.begin();
    ecg_init();
    ui_init();

    Serial.println("System ready.");
    Serial.println("Button: GPIO2  |  ECG: AD8232  |  OLED: SW I2C D4/D5");
}

void loop() {
    // 1) Опрос кнопки — как можно чаще, без блокировок
    myButton.update();

    // Обработка короткого нажатия (перемещение стрелочки)
    if (myButton.isShortPress()) {
        Serial.println("[BTN] Short");
        ui_handleShortPress();
    }

    // Обработка двойного нажатия (вход/выход из подменю)
    if (myButton.isDoublePress()) { 
        Serial.println("[BTN] Double");
        ui_handleDoublePress();
    }

    uint32_t now = millis();

    // 2) Сэмплирование ЭКГ по таймеру (независимо от UI)
    if (now - lastEcgTime >= ECG_PERIOD_MS) {
        lastEcgTime = now;
        if (ecg_is_enabled()) {
            int sample = ecg_read_sample();
            Serial.println(sample);
        }
    }

    // 3) Обновление UI ~10 Гц
    if (now - lastUiTime >= UI_PERIOD_MS) {
        lastUiTime = now;
        ui_update();
    }
}
