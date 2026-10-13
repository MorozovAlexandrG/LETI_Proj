#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

// SW I2C на D4 (SDA) и D5 (SCL) — не конфликтует с AD8232
#define OLED_SDA D4
#define OLED_SCL D5

extern U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;

enum MenuState {
    MAIN_MENU,
    ECG_MENU,
    DATA_MENU
};

extern MenuState currentState;
extern int mainMenuSelection;   // 0 = ECG, 1 = Data
extern int ecgMenuSelection;    // 0 = OFF, 1 = ON

void ui_init();
void ui_update();               // вызывать по таймеру (~10 Гц)

void ui_handleShortPress();
void ui_handleDoublePress();
void ui_handleLongPress();