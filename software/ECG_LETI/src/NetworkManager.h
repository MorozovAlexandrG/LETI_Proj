#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

enum WiFiConnectStatus { WIFI_SEARCHING, WIFI_CONNECTED, WIFI_FAILED };

class NetworkManager {
private:
    WebServer server;
    DNSServer dnsServer;
    Preferences prefs;
    
    bool isConfiguring = false;
    unsigned long configStartTime = 0;
    const unsigned long CONFIG_TIMEOUT = 120000;

    const char* apName = "ECG-Config";
    const char* apPassword = "12345678";

    void handleRoot();
    void handleSave();
    void generateHtmlPage(String& page);

public:
    NetworkManager();
    void begin();
    void startConfigPortal();
    void stopConfigPortal(); // НОВОЕ: Остановка портала по кнопке отмены
    void handleClient();
    
    bool hasSavedWiFi();     // НОВОЕ: Проверка, есть ли сохраненная сеть
    bool isConnectedToHomeWiFi();
    WiFiConnectStatus connectToSavedWiFi();
    void disconnectWiFi();
};