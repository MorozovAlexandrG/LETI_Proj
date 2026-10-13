#pragma once
#include <Arduino.h>
#include <U8g2lib.h>

enum MenuState { 
    MAIN_MENU, SUBMENU_ECG, SUBMENU_WIFI_CONFIG, 
    MENU_RECORDS, MENU_PATIENT, SCREEN_SENDING 
};

class SimpleMenu {
private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    
    MenuState currentState = MAIN_MENU;
    
    int mainCurrentItem = 0;
    int topIndex = 0; 
    const int totalMainItems = 3;
    const char* menuItems[3] = { "1. Start ECG", "2. Wi-Fi", "3. Send Data" };

    int subCurrentItem = 0; 
    bool isEcgOn = false;
    bool hasSavedWiFi = false; 

    int recordIndex = 0;
    const int totalRecords = 3;
    const char* records[3] = { "1. 20 Sep", "2. 21 Sep", "3. 22 Sep" };

    // НОВОЕ: Выбор в меню пациента (0 = Назад, 1 = Отправить)
    int patientCurrentItem = 0; 

    bool wifiSetupRequested = false;
    bool sendRequested = false;

    void drawMainMenu();
    void drawSubmenu(const char* title);
    void drawRecordsMenu();
    void drawPatientMenu(); // Обновленная версия

public:
    SimpleMenu();
    void begin();
    void draw();
    void nextItem();
    void selectItem();
    
    void drawWifiSetupScreen(int animFrame);
    void resetToMainMenu();
    
    void updateWiFiStatus(bool saved) { hasSavedWiFi = saved; }
    
    bool isWifiSetupRequested() const { return wifiSetupRequested; }
    void clearWifiSetupRequest() { wifiSetupRequested = false; }
    
    bool isSendRequested() const { return sendRequested; }
    void clearSendRequest() { sendRequested = false; }
    
    int getCurrentMainItem() const { return mainCurrentItem; }
    bool isEcgActive() const { return isEcgOn; }
};