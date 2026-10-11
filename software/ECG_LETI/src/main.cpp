#include <Arduino.h>
#include <OneButton.h>
#include "DisplayManager.h"

// Пин кнопки (на XIAO ESP32-C3 удобно использовать GPIO 3 или 4)
#define BUTTON_PIN 4

// Создаем объекты
SimpleMenu myMenu;
// OneButton(пин, activeLow=true, pullupActive=true)
OneButton button(BUTTON_PIN, true, true); 

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("System started");

    // Инициализация меню
    myMenu.begin();

    // Привязываем обработчики кнопки к методам меню
    button.attachClick([]() {
        Serial.println("Click -> Next Item");
        myMenu.nextItem();
    });

    button.attachDoubleClick([]() {
        Serial.println("Double Click -> Select");
        myMenu.selectItem();
    });
    
    // Опционально: обработка долгого нажатия для возврата в главное меню из любого места
    button.attachLongPressStop([]() {
        Serial.println("Long Press -> Back to Main Menu");
        // Можно добавить метод myMenu.goToMainMenu(), если потребуется
    });
}

void loop() {
    // Библиотека OneButton должна постоянно опрашиваться
    button.tick();

    // Здесь позже можно добавить логику, которая работает, когда isEcgActive() == true
    // Например, опрос датчика AD8232
    
    delay(10); 
}