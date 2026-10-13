#include "button.h"

Button::Button(int pin) : pin(pin) {}

void Button::begin() {
    pinMode(pin, INPUT_PULLUP);
    // Передаем 'this' в качестве аргумента, чтобы прерывание знало, какой объект вызывать
    attachInterruptArg(digitalPinToInterrupt(pin), &Button::isrThunk, this, FALLING);
}

// Статический метод принимает указатель на объект и вызывает его внутренний нестатический метод
void IRAM_ATTR Button::isrThunk(void* arg) {
    static_cast<Button*>(arg)->handleISR();
}

// Внутренний метод объекта имеет полный доступ к private-полям
void IRAM_ATTR Button::handleISR() {
    isrClickDetected = true;
}

void Button::update() {
    uint32_t now = millis();

    // 1) Проверяем флаг из прерывания
    if (isrClickDetected) {
        isrClickDetected = false;

        // Фильтрация дребезга контактов провода
        if (now - isrLastClickTime >= LOCKOUT_MS) {
            clickCount++;
            lastClickReleaseTime = now;
            isrLastClickTime = now;
        }
    }

    // 2) Тайм-аут ожидания: одиночный клик или двойной
    if (clickCount > 0) {
        if (now - lastClickReleaseTime >= DOUBLE_CLICK_MS) {
            if (clickCount == 1) {
                shortPressed = true;
            } else if (clickCount >= 2) {
                doublePressed = true;
            }
            clickCount = 0; // Сброс счетчика
        }
    }
}

bool Button::isShortPress() {
    if (shortPressed) { shortPressed = false; return true; }
    return false;
}

bool Button::isDoublePress() {
    if (doublePressed) { doublePressed = false; return true; }
    return false;
}
