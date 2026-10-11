#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

// Состояния, в которых может находиться меню
enum MenuState { MAIN_MENU, SUBMENU_ECG, SUBMENU_WIFI };

class SimpleMenu {
private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    
    MenuState currentState = MAIN_MENU;
    
    // Переменные для главного меню
    int mainCurrentItem = 0;
    const int totalMainItems = 2;
    const char* menuItems[2] = { "1. Start ECG", "2. Wi-Fi Setup" };

    // Переменные для подменю
    int subCurrentItem = 0; // 0 = OFF, 1 = ON
    
    // Фактические состояния устройств
    bool isEcgOn = false;
    bool isWifiOn = false;

    // Внутренние методы отрисовки
    void drawMainMenu();
    void drawSubmenu(const char* title);
    void drawHighlightMain();
    void drawHighlightSub();

public:
    SimpleMenu();
    void begin();
    void draw();          // Обновляет экран в зависимости от currentState
    
    // Эти методы вызываются из обработчиков кнопки в main.cpp
    void nextItem();      // Одинарный клик
    void selectItem();    // Двойной клик
    
    // Геттеры для main.cpp, чтобы знать, что делать
    bool isEcgActive() const { return isEcgOn; }
    bool isWifiActive() const { return isWifiOn; }
};