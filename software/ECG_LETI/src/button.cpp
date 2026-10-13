#include "button.h"

Button::Button(int pin) : pin(pin) {}

void Button::begin() {
    pinMode(pin, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(pin), &Button::isrThunk, this, FALLING); // Ловим только переход в LOW
}

void IRAM_ATTR Button::isrThunk(void* arg) {
    static_cast<Button*>(arg)->handleISR();
}

void IRAM_ATTR Button::handleISR() {
    uint32_t now = millis();
    
    // Если прошло больше времени блокировки, фиксируем ЧИСТОЕ новое касание
    if (now - isrLastClickTime >= LOCKOUT_MS) {
        isrClickDetected = true;
        isrLastClickTime = now; // Перезапускаем таймер блокировки дребезга
    }
}

void Button::update() {
    // Атомарно забираем флаг из прерывания
    if (isrClickDetected) {
        isrClickDetected = false;
        shortPressed = true; // Мгновенно взводим флаг для main.cpp
    }
}

bool Button::isShortPress() {
    if (shortPressed) { 
        shortPressed = false; 
        return true; 
    }
    return false;
}
