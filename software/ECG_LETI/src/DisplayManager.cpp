#include "DisplayManager.h"
#include "heart.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(
    U8G2_R0,
    /* clock=*/ OLED_SCL,
    /* data=*/  OLED_SDA,
    /* reset=*/ U8X8_PIN_NONE
);

MenuState currentState = MAIN_MENU;
int mainMenuSelection = 0;
int ecgMenuSelection  = 0;

void ui_init() {
    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
}

void ui_update() {
    u8g2.clearBuffer();

    switch (currentState) {
    case MAIN_MENU: {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 12, "Main menu");

        u8g2.setFont(u8g2_font_6x10_tr);
        char buf[32];
        snprintf(buf, sizeof(buf), "%c1) ECG: %s",
                 mainMenuSelection == 0 ? '>' : ' ',
                 ecg_is_enabled() ? "ON" : "OFF");
        u8g2.drawStr(0, 32, buf);

        snprintf(buf, sizeof(buf), "%c2) Data",
                 mainMenuSelection == 1 ? '>' : ' ');
        u8g2.drawStr(0, 47, buf);

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(0, 60, "Short:Move  Dbl:Enter  Long:Back");
        break;
    }
    case ECG_MENU: {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 12, "ECG Settings");

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(0, 32, "Status:");

        if (ecgMenuSelection == 0)
            u8g2.drawStr(0, 48, "> OFF    ON");
        else
            u8g2.drawStr(0, 48, "  OFF   >ON");

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(0, 60, "Short:Toggle  Dbl:Save&Back");
        break;
    }
    case DATA_MENU: {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 12, "Data");

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(0, 35, "Empty for now.");

        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(0, 60, "Dbl/Long: Back");
        break;
    }
    }

    u8g2.sendBuffer();
}

void ui_handleShortPress() {
    if (currentState == MAIN_MENU) {
        mainMenuSelection = (mainMenuSelection == 0) ? 1 : 0;
    } else if (currentState == ECG_MENU) {
        ecgMenuSelection = (ecgMenuSelection == 0) ? 1 : 0;
    }
}

void ui_handleDoublePress() {
    if (currentState == MAIN_MENU) {
        if (mainMenuSelection == 0) {
            currentState = ECG_MENU;
            ecgMenuSelection = ecg_is_enabled() ? 1 : 0;
        } else {
            currentState = DATA_MENU;
        }
    } else if (currentState == ECG_MENU) {
        if (ecgMenuSelection == 1) ecg_enable();
        else                      ecg_disable();
        currentState = MAIN_MENU;
    } else if (currentState == DATA_MENU) {
        currentState = MAIN_MENU;
    }
}

void ui_handleLongPress() {
    if (currentState != MAIN_MENU) {
        currentState = MAIN_MENU;
    }
}