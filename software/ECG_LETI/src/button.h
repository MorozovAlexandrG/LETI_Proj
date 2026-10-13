#pragma once
#include <Arduino.h>

class Button {
public:
    Button(int pin);
    void begin();
    void update();
    bool isShortPress(); // Любое касание вызовет true здесь

private:
    int pin;

    volatile uint32_t isrLastClickTime = 0;
    volatile bool     isrClickDetected = false;

    bool shortPressed = false;

    // Время полной блокировки порта после касания в миллисекундах.
    // 200 мс — идеальное время, чтобы человек успел коснуться и убрать провод 
    // без риска словить повторный ложный клик от дребезга.
    const uint16_t LOCKOUT_MS = 200; 

    void IRAM_ATTR handleISR();
    static void IRAM_ATTR isrThunk(void* arg);
};
