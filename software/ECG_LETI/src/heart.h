#pragma once
#include <Arduino.h>

// === Пины ===
#define ECG_AD_OUT  3     // Аналоговый выход AD8232 (D1 на XIAO ESP32-C3)
#define ECG_LO_MINUS 4    // Leads-off - (D2)
#define ECG_LO_PLUS  5    // Leads-off + (D3)
#define ECG_SDN     21    // Shutdown (D6)

void ecg_init();
void ecg_enable();
void ecg_disable();
bool ecg_is_enabled();

// Читает один сэмпл и возвращает значение (0 если лиды отвалились)
int  ecg_read_sample();

// Возвращает true, если лиды отвалились
bool ecg_leads_off();