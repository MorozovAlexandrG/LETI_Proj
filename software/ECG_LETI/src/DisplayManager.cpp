#include "DisplayManager.h"

SimpleMenu::SimpleMenu() : u8g2(U8G2_R0, U8X8_PIN_NONE) {}

void SimpleMenu::begin() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tf);
    draw();
}

void SimpleMenu::resetToMainMenu() {
    currentState = MAIN_MENU;
    draw();
}

void SimpleMenu::draw() {
    switch (currentState) {
        case MAIN_MENU: drawMainMenu(); break;
        case SUBMENU_ECG: drawSubmenu("Start ECG"); break;
        case SUBMENU_WIFI_CONFIG: drawSubmenu("Wi-Fi Setup"); break;
        case MENU_RECORDS: drawRecordsMenu(); break;
        case MENU_PATIENT: drawPatientMenu(); break;
    }
}

void SimpleMenu::drawMainMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, "Main Menu");
    u8g2.drawHLine(0, 14, 128);

    const int MAX_VISIBLE = 3;
    const int startY = 28;
    const int spacing = 16;

    if (mainCurrentItem >= topIndex + MAX_VISIBLE) topIndex = mainCurrentItem - MAX_VISIBLE + 1;
    if (mainCurrentItem < topIndex) topIndex = mainCurrentItem;

    u8g2.setFont(u8g2_font_7x14_tf);
    
    for (int i = 0; i < MAX_VISIBLE; i++) {
        int dataIndex = topIndex + i;
        if (dataIndex >= totalMainItems) break; 
        
        int yPos = startY + (i * spacing);
        u8g2.drawStr(5, yPos, menuItems[dataIndex]);
        
        // ИСПРАВЛЕНО: Используем простой текст "OK" или "X" вместо шрифта иконок. 
        // Это гарантирует компиляцию на любой версии U8g2.
        if (dataIndex == 1) {
            u8g2.setFont(u8g2_font_6x10_tf);
            u8g2.drawStr(110, yPos, hasSavedWiFi ? "OK" : "X");
            u8g2.setFont(u8g2_font_7x14_tf); // Возвращаем основной шрифт
        }
        
        if (dataIndex == mainCurrentItem) {
            u8g2.drawRFrame(0, yPos - 12, 128, 16, 3);
        }
    }
    u8g2.sendBuffer();
}

void SimpleMenu::drawSubmenu(const char* title) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, title);
    u8g2.drawHLine(0, 14, 128);
    u8g2.drawRFrame(44, 25, 40, 20, 10); 
    if (subCurrentItem == 0) u8g2.drawDisc(54, 35, 8); 
    else u8g2.drawDisc(74, 35, 8); 
    u8g2.setFont(u8g2_font_7x14_tf);
    u8g2.drawStr(30, 58, "OFF");
    u8g2.drawStr(82, 58, "ON");
    if (subCurrentItem == 0) u8g2.drawRFrame(25, 46, 30, 16, 3);
    else u8g2.drawRFrame(75, 46, 25, 16, 3);
    u8g2.sendBuffer();
}

void SimpleMenu::drawRecordsMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, "Select Recording");
    u8g2.drawHLine(0, 14, 128);

    u8g2.setFont(u8g2_font_7x14_tf);
    for (int i = 0; i < totalRecords; i++) {
        u8g2.drawStr(5, 32 + (i * 18), records[i]);
    }
    
    int yPos = 32 + (recordIndex * 18);
    u8g2.drawRFrame(0, yPos - 14, 128, 18, 3);
    u8g2.sendBuffer();
}

// === ИСПРАВЛЕНО: Стрелка рисуется геометрически (треугольником), 0 зависимостей от шрифтов ===
void SimpleMenu::drawPatientMenu() {
    u8g2.clearBuffer();
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, "Patient Info");
    u8g2.drawHLine(0, 14, 128);

    // Рамка выделения вокруг области стрелки
    if (patientCurrentItem == 0) {
        u8g2.drawRFrame(2, 16, 14, 16, 3); 
    }
    
    // Рисуем стрелку "Назад" (треугольник, указывающий влево)
    // Аргументы: x0,y0 (вершина), x1,y1 (верхний правый угол), x2,y2 (нижний правый угол)
    u8g2.drawTriangle(6, 24, 14, 18, 14, 30);

    u8g2.setFont(u8g2_font_7x14_tf);
    u8g2.drawStr(25, 30, "Name: Ivan I.");
    u8g2.drawStr(25, 46, "ID: #849201");

    if (patientCurrentItem == 1) {
        u8g2.drawRFrame(10, 52, 108, 12, 2);
    }
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(25, 62, "[ SEND DATA ]");
    
    u8g2.sendBuffer();
}

void SimpleMenu::drawWifiSetupScreen(int animFrame) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(5, 10, "Wi-Fi Setup");
    u8g2.drawHLine(0, 14, 128);

    u8g2.setFont(u8g2_font_7x14_tf);
    const char* txt = "Setup on phone";
    int w = u8g2.getStrWidth(txt);
    u8g2.drawStr((128 - w) / 2, 58, txt);

    int cx = 64, cy = 30, r = 10;
    int px[4] = { cx, cx + r, cx, cx - r };
    int py[4] = { cy - r, cy, cy + r, cy };
    
    for (int i = 0; i < 4; i++) {
        if (i == animFrame) u8g2.drawDisc(px[i], py[i], 3);
        else u8g2.drawCircle(px[i], py[i], 2);
    }
    
    // Иконка крестика (отмена) в правом нижнем углу
    u8g2.drawLine(115, 55, 123, 63);
    u8g2.drawLine(123, 55, 115, 63);
    u8g2.drawFrame(112, 52, 14, 14);
    
    u8g2.sendBuffer();
}

void SimpleMenu::nextItem() {
    if (currentState == MAIN_MENU) mainCurrentItem = (mainCurrentItem + 1) % totalMainItems;
    else if (currentState == SUBMENU_ECG || currentState == SUBMENU_WIFI_CONFIG) subCurrentItem = (subCurrentItem == 0) ? 1 : 0;
    else if (currentState == MENU_RECORDS) recordIndex = (recordIndex + 1) % totalRecords;
    else if (currentState == MENU_PATIENT) {
        patientCurrentItem = (patientCurrentItem == 0) ? 1 : 0; 
    }
    draw();
}

void SimpleMenu::selectItem() {
    if (currentState == MAIN_MENU) {
        if (mainCurrentItem == 0) currentState = SUBMENU_ECG;
        else if (mainCurrentItem == 1) {
            currentState = SUBMENU_WIFI_CONFIG;
            subCurrentItem = 0;
        }
        else if (mainCurrentItem == 2) currentState = MENU_RECORDS;
    } 
    else if (currentState == SUBMENU_ECG) {
        isEcgOn = (subCurrentItem == 1);
        currentState = MAIN_MENU;
    } 
    else if (currentState == SUBMENU_WIFI_CONFIG) {
        if (subCurrentItem == 1) wifiSetupRequested = true;
        currentState = MAIN_MENU;
    }
    else if (currentState == MENU_RECORDS) {
        currentState = MENU_PATIENT;
        patientCurrentItem = 0; // Сброс на "Назад" при входе
    }
    else if (currentState == MENU_PATIENT) {
        if (patientCurrentItem == 0) {
            currentState = MENU_RECORDS; // Шаг назад
        } else {
            sendRequested = true; // Запрос отправки
        }
    }
    draw();
}