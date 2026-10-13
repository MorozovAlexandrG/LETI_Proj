#pragma once
#include <Arduino.h>

class Button {
public:
    Button(int pin);
    void begin();
    void update();
    
    bool isShortPress();
    bool isDoublePress();

private:
    int pin;

    // Переменные для связи с прерыванием
    volatile uint32_t isrLastClickTime = 0;
    volatile bool     isrClickDetected = false;

    // Переменные распознавания кликов в основном цикле
    uint8_t clickCount = 0;
    uint32_t lastClickReleaseTime = 0;

    // Флаги событий для главного цикла
    bool shortPressed = false;
    bool doublePressed = false;

    // Константы таймингов (в миллисекундах)
    const uint16_t LOCKOUT_MS = 180;      // Защита от дребезга контактов
    const uint16_t DOUBLE_CLICK_MS = 350; // Время ожидания второго клика

    // Прерывания (ISR)
    void IRAM_ATTR handleISR();
    static void IRAM_ATTR isrThunk(void* arg); // Метод-переходник для attachInterruptArg
};
