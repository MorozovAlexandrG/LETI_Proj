#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

class WireButton {
public:
    WireButton(uint8_t pin);
    void begin();
    void update();
    bool isLongPressed(unsigned long durationMs);

private:
    uint8_t _pin;
    bool _currentState;
    int _lastState;
    unsigned long _pressStartTime;
    bool _longPressTriggered;
    unsigned long _lastDebounceTime;
};

#endif
