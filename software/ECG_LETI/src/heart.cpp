#include "heart.h"

static bool isEnabled = false;

void ecg_init() {
    pinMode(ECG_LO_MINUS, INPUT);
    pinMode(ECG_LO_PLUS,  INPUT);
    pinMode(ECG_SDN,      OUTPUT);

    analogReadResolution(12);
    

    ecg_disable();
}

void ecg_enable() {
    digitalWrite(ECG_SDN, HIGH);
    isEnabled = true;
}

void ecg_disable() {
    digitalWrite(ECG_SDN, LOW);
    isEnabled = false;
}

bool ecg_is_enabled() {
    return isEnabled;
}

bool ecg_leads_off() {
    return digitalRead(ECG_LO_MINUS) || digitalRead(ECG_LO_PLUS);
}

int ecg_read_sample() {
    if (!isEnabled) return 0;
    if (ecg_leads_off()) return 0; // по соглашению — 0 значит "нет сигнала"
    return analogRead(ECG_AD_OUT);
}