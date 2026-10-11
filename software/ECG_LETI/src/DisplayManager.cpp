#include "DisplayManager.h"

SimpleMenu::SimpleMenu() : u8g2(U8G2_R0, U8X8_PIN_NONE) {}

void SimpleMenu::begin() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tf);
    draw(); // Рисуем начальное состояние
}

void SimpleMenu::draw() {
    if (currentState == MAIN_MENU) {
        drawMainMenu();
    } else {
        const char* title = (currentState == SUBMENU_ECG) ? "Start ECG" : "Wi-Fi Setup";
        drawSubmenu(title);
    }
}

// --- ОТРИСОВКА ГЛАВНОГО МЕНЮ ---
void SimpleMenu::drawMainMenu() {
    u8g2.clearBuffer();
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, "Main Menu");
    u8g2.drawHLine(0, 14, 128);

    u8g2.setFont(u8g2_font_7x14_tf);
    const int startY = 32;
    const int spacing = 22;

    for (int i = 0; i < totalMainItems; i++) {
        u8g2.drawStr(5, startY + (i * spacing), menuItems[i]);
    }

    drawHighlightMain();
    u8g2.sendBuffer();
}

void SimpleMenu::drawHighlightMain() {
    const int startY = 32;
    const int spacing = 22;
    int yPos = startY + (mainCurrentItem * spacing);
    u8g2.drawRFrame(0, yPos - 14, 128, 18, 3);
}

// --- ОТРИСОВКА ПОДМЕНЮ ---
void SimpleMenu::drawSubmenu(const char* title) {
    u8g2.clearBuffer();

    // 1. Заголовок
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, title);
    u8g2.drawHLine(0, 14, 128);

    // 2. Переключатель (Toggle Switch) по центру
    // Фон переключателя
    u8g2.drawRFrame(44, 25, 40, 20, 10); 
    
    // Ползунок (зависит от subCurrentItem: 0=слева, 1=справа)
    if (subCurrentItem == 0) {
        u8g2.drawDisc(54, 35, 8); // OFF (левая позиция)
    } else {
        u8g2.drawDisc(74, 35, 8); // ON (правая позиция)
    }

    // 3. Надписи OFF и ON внизу
    u8g2.setFont(u8g2_font_7x14_tf);
    u8g2.drawStr(30, 58, "OFF");
    u8g2.drawStr(82, 58, "ON");

    // 4. Рамка выделения вокруг OFF или ON
    drawHighlightSub();

    u8g2.sendBuffer();
}

void SimpleMenu::drawHighlightSub() {
    // Рамка вокруг OFF (x=25, y=46, w=30, h=16) или ON (x=75, y=46, w=25, h=16)
    if (subCurrentItem == 0) {
        u8g2.drawRFrame(25, 46, 30, 16, 3);
    } else {
        u8g2.drawRFrame(75, 46, 25, 16, 3);
    }
}

// --- ЛОГИКА УПРАВЛЕНИЯ ---

void SimpleMenu::nextItem() {
    if (currentState == MAIN_MENU) {
        mainCurrentItem++;
        if (mainCurrentItem >= totalMainItems) mainCurrentItem = 0;
    } else {
        // В подменю переключаем между 0 (OFF) и 1 (ON)
        subCurrentItem = (subCurrentItem == 0) ? 1 : 0;
    }
    draw();
}

void SimpleMenu::selectItem() {
    if (currentState == MAIN_MENU) {
        // Проваливаемся в подменю
        if (mainCurrentItem == 0) {
            currentState = SUBMENU_ECG;
            subCurrentItem = isEcgOn ? 1 : 0; // Ставим выделение на текущее состояние
        } else {
            currentState = SUBMENU_WIFI;
            subCurrentItem = isWifiOn ? 1 : 0;
        }
    } else {
        // Мы в подменю: применяем выбор и возвращаемся в главное меню
        if (currentState == SUBMENU_ECG) {
            isEcgOn = (subCurrentItem == 1);
            Serial.println(isEcgOn ? "ACTION: ECG Turned ON" : "ACTION: ECG Turned OFF");
        } else if (currentState == SUBMENU_WIFI) {
            isWifiOn = (subCurrentItem == 1);
            Serial.println(isWifiOn ? "ACTION: Wi-Fi Turned ON" : "ACTION: Wi-Fi Turned OFF");
        }
        currentState = MAIN_MENU; // Возврат назад
    }
    draw();
}