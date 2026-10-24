#include "ECG_Recorder.h"

ECG_Recorder::ECG_Recorder() {
    _state = STATE_OFF;
    _fileIndex = 1;
    _sdAvailable = false;
    _bufferIndex = 0;
    _currentWriteBuffer = _bufferA;
    _needFlushA = false;
    _needFlushB = false;
}

void ECG_Recorder::begin() {
    pinMode(PIN_ECG_SDN, OUTPUT);
    pinMode(PIN_ECG_LO_MINUS, INPUT);
    pinMode(PIN_ECG_LO_PLUS, INPUT);
    
    powerOffSensor(); // AD8232 изначально полностью выключен!

    SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    if (!SD.begin(PIN_SD_CS)) {
        _sdAvailable = false;
    } else {
        _sdAvailable = true;
    }
}

void ECG_Recorder::update() {
    if (_state != STATE_RECORDING) return;

    // 1. Сбор данных строго по таймеру (каждые 20 мс)
    unsigned long currentMicros = micros();
    if (currentMicros - _lastSampleTime >= ECG_SAMPLE_INTERVAL) {
        _lastSampleTime += ECG_SAMPLE_INTERVAL; 

        bool leadsOff = digitalRead(PIN_ECG_LO_MINUS) || digitalRead(PIN_ECG_LO_PLUS);
        int ecgValue = leadsOff ? 0 : analogRead(PIN_ECG_OUT);

        // Кладём во временную память ОЗУ
        _currentWriteBuffer[_bufferIndex].timeMs = millis() - _stateStartTime;
        _currentWriteBuffer[_bufferIndex].value = ecgValue;
        _bufferIndex++;

        // Вывод ТОЛЬКО чистого числа для Python (скрипт примет его за секунду)
        Serial.println(ecgValue);

        // Переключение буферов при заполнении
        if (_bufferIndex >= BUFFER_SIZE) {
            _bufferIndex = 0;
            if (_currentWriteBuffer == _bufferA) {
                _needFlushA = true;
                _currentWriteBuffer = _bufferB;
            } else {
                _needFlushB = true;
                _currentWriteBuffer = _bufferA;
            }
        }
    }

    // 2. Фоновый переброс на SD-карту (когда процессор свободен между измерениями)
    if (_needFlushA) {
        flushBuffer(_bufferA, BUFFER_SIZE);
        _needFlushA = false;
    }
    if (_needFlushB) {
        flushBuffer(_bufferB, BUFFER_SIZE);
        _needFlushB = false;
    }
}

void ECG_Recorder::triggerStart() {
    if (_state == STATE_OFF) {
        if (!_sdAvailable && !SD.begin(PIN_SD_CS)) {
            return; // Без карты памяти старт блокируется
        }
        _sdAvailable = true;

        // Автоматический подбор имени файла (ищет свободный индекс ecg_XXX.csv)
        char fileName[32];
        do {
            snprintf(fileName, sizeof(fileName), "/ecg_%03d.csv", _fileIndex++);
        } while (SD.exists(fileName));

        _dataFile = SD.open(fileName, FILE_WRITE);
        if (!_dataFile) return;
        
        _dataFile.println("Time_ms,ECG_Value");
        
        _bufferIndex = 0;
        _currentWriteBuffer = _bufferA;
        _needFlushA = false;
        _needFlushB = false;

        powerOnSensor(); // ВКЛЮЧАЕМ SDN датчика (LOW)
        
        _lastSampleTime = micros();
        _stateStartTime = millis();
        _state = STATE_RECORDING;
    }
}

void ECG_Recorder::triggerStop() {
    if (_state == STATE_RECORDING) {
        powerOffSensor(); // Мгновенно ВЫКЛЮЧАЕМ SDN датчика (HIGH)
        _state = STATE_OFF;

        // Записываем «хвост» данных, оставшийся в активном буфере
        if (_bufferIndex > 0) {
            flushBuffer(_currentWriteBuffer, _bufferIndex);
        }

        if (_dataFile) {
            _dataFile.close();
        }
    }
}

void ECG_Recorder::powerOnSensor() {
    digitalWrite(PIN_ECG_SDN, LOW); 
    delay(150); // Время для стабилизации операционных усилителей
}

void ECG_Recorder::powerOffSensor() {
    digitalWrite(PIN_ECG_SDN, HIGH); 
}

void ECG_Recorder::flushBuffer(ECGSample* buffer, uint16_t size) {
    if (_sdAvailable && _dataFile) {
        char dataBuf[32];
        for (uint16_t i = 0; i < size; i++) {
            int len = snprintf(dataBuf, sizeof(dataBuf), "%lu,%u\n", buffer[i].timeMs, buffer[i].value);
            if (len > 0) {
                _dataFile.write((uint8_t*)dataBuf, len);
            }
        }
        _dataFile.flush(); // Фиксирует данные в файловой системе (безопасность при разряде)
    }
}
