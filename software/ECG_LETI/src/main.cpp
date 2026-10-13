#include <Arduino.h>
#include <OneButton.h>
#include "DisplayManager.h"
#include "NetworkManager.h"

#define BUTTON_PIN 4

SimpleMenu myMenu;
OneButton button(BUTTON_PIN, true, true);
NetworkManager network;

bool isWifiConfigMode = false;
unsigned long lastAnimTime = 0;
int animFrame = 0;

void handleClick() {
    if (isWifiConfigMode) {
        network.stopConfigPortal();
        isWifiConfigMode = false;
        myMenu.resetToMainMenu();
        return;
    }
    myMenu.nextItem();
}

void handleDoubleClick() {
    if (isWifiConfigMode) return;
    
    myMenu.selectItem();
    
    // ИСПРАВЛЕНО: Убрана двойная проверка условия
    if (myMenu.isWifiSetupRequested()) {
        myMenu.clearWifiSetupRequest();
        isWifiConfigMode = true;
        network.startConfigPortal();
        animFrame = 0;
        lastAnimTime = millis();
        myMenu.drawWifiSetupScreen(animFrame);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- ECG Project Start ---");

    myMenu.begin();
    network.begin();
    
    // ИСПРАВЛЕНО: Обновляем статус Wi-Fi при каждом старте
    myMenu.updateWiFiStatus(network.hasSavedWiFi());
    
    button.attachClick(handleClick);
    button.attachDoubleClick(handleDoubleClick);
    
    myMenu.draw();
}

void loop() {
    button.tick();

    if (isWifiConfigMode) {
        network.handleClient();
        
        if (millis() - lastAnimTime > 250) {
            animFrame = (animFrame + 1) % 4;
            myMenu.drawWifiSetupScreen(animFrame);
            lastAnimTime = millis();
        }
    }
    
    delay(10);
}