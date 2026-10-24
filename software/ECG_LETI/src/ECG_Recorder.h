#ifndef ECG_RECORDER_H
#define ECG_RECORDER_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"

enum ECGState {
    STATE_OFF,
    STATE_RECORDING
};

struct ECGSample {
    uint32_t timeMs;
    uint16_t value;
};

class ECG_Recorder {
public:
    ECG_Recorder();
    void begin();
    void update();
    void triggerStart();
    void triggerStop();
    ECGState getState() { return _state; }

private:
    void powerOnSensor();
    void powerOffSensor();
    void flushBuffer(ECGSample* buffer, uint16_t size);

    ECGState _state;
    uint16_t _fileIndex;
    bool _sdAvailable;
    File _dataFile;

    unsigned long _lastSampleTime;
    unsigned long _stateStartTime;

    // Пинг-понг буферы в RAM
    ECGSample _bufferA[BUFFER_SIZE];
    ECGSample _bufferB[BUFFER_SIZE];
    
    ECGSample* _currentWriteBuffer;
    uint16_t _bufferIndex;

    bool _needFlushA;
    bool _needFlushB;
};

#endif
