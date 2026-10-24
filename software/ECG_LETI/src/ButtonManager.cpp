#include "ButtonManager.h"

WireButton::WireButton(uint8_t pin) {
    _pin = pin;
    _pressStartTime = 0;
    _longPressTriggered = false;
    _lastDebounceTime = 0;
    _currentState = false;
    _lastState = HIGH;
}

void WireButton::begin() {
    pinMode(_pin, INPUT_PULLUP);
    _lastState = digitalRead(_pin);
    _currentState = (_lastState == LOW);
}

void WireButton::update() {
    int reading = digitalRead(_pin);

    if (reading != _lastState) {
        _lastDebounceTime = millis();
    }

    if ((millis() - _lastDebounceTime) > 50) {
        bool stableState = (reading == LOW);

        if (stableState != _currentState) {
            _currentState = stableState;
            if (_currentState) {
                _pressStartTime = millis();
                _longPressTriggered = false;
            } else {
                _pressStartTime = 0;
            }
        }
    }
    _lastState = reading;
}

bool WireButton::isLongPressed(unsigned long durationMs) {
    if (_currentState && !_longPressTriggered && _pressStartTime > 0) {
        if (millis() - _pressStartTime >= durationMs) {
            _longPressTriggered = true;
            return true;
        }
    }
    return false;
}
