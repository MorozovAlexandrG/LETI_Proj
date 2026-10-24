#include "NetworkManager.h"

NetworkManager::NetworkManager() : server(80) {}

void NetworkManager::begin() {
    prefs.begin("wifi_config", false);
}

bool NetworkManager::hasSavedWiFi() {
    return prefs.isKey("ssid") && prefs.getString("ssid", "") != "";
}

bool NetworkManager::isConnectedToHomeWiFi() {
    return (WiFi.status() == WL_CONNECTED);
}

void NetworkManager::startConfigPortal() {
    Serial.println("Starting Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, apPassword);
    
    dnsServer.start(53, "*", WiFi.softAPIP());

    server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
    server.on("/save", HTTP_POST, [this]() { this->handleSave(); });
    server.onNotFound([this]() { this->handleRoot(); });

    server.begin();
    isConfiguring = true;
    configStartTime = millis();
    Serial.println("Config Portal started.");
}

// НОВОЕ: Корректное выключение точки доступа
void NetworkManager::stopConfigPortal() {
    if (isConfiguring) {
        dnsServer.stop();
        server.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        isConfiguring = false;
        Serial.println("Config portal stopped by user.");
    }
}

void NetworkManager::handleClient() {
    if (!isConfiguring) return;

    dnsServer.processNextRequest();
    server.handleClient();

    if (millis() - configStartTime > CONFIG_TIMEOUT) {
        Serial.println("Config portal timeout.");
        stopConfigPortal(); // Просто выключаем, не перезагружая устройство
    }
}

void NetworkManager::generateHtmlPage(String& page) {
    page = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
    page += "<style>body{font-family: Arial; text-align: center; margin-top: 50px;}";
    page += "input{padding: 10px; margin: 10px; width: 80%;} button{padding: 10px 20px; background: #007bff; color: white; border: none; border-radius: 5px;}</style>";
    page += "</head><body>";
    page += "<h2>ECG Monitor Setup</h2>";
    page += "<form action='/save' method='POST'>";
    page += "<input type='text' name='ssid' placeholder='Wi-Fi Name (SSID)' required><br>";
    page += "<input type='password' name='pass' placeholder='Wi-Fi Password' required><br>";
    page += "<button type='submit'>Connect & Save</button>";
    page += "</form></body></html>";
}

void NetworkManager::handleRoot() {
    String html;
    generateHtmlPage(html);
    server.send(200, "text/html", html);
}

void NetworkManager::handleSave() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    server.send(200, "text/html", "<html><body><h2>Saved! Rebooting...</h2></body></html>");
    delay(2000);
    ESP.restart();
}

WiFiConnectStatus NetworkManager::connectToSavedWiFi() {
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");

    if (ssid == "") return WIFI_FAILED;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) return WIFI_CONNECTED;
    return WIFI_FAILED;
}

void NetworkManager::disconnectWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}