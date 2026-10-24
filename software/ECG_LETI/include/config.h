#ifndef CONFIG_H
#define CONFIG_H

// --- Пины AD8232 ---
#define PIN_ECG_OUT      2   // GPIO2
#define PIN_ECG_LO_PLUS  3   // GPIO3
#define PIN_ECG_LO_MINUS 4   // GPIO4
#define PIN_ECG_SDN      5   // GPIO5

// --- Пины MicroSD (SPI) ---
#define PIN_SD_CS        6   // GPIO6
#define PIN_SD_MOSI      10  // GPIO10
#define PIN_SD_MISO      9   // GPIO9
#define PIN_SD_SCK       8   // GPIO8

// --- Пины управления (Провода, замыкаемые на GND) ---
#define PIN_WIRE_START   7   // GPIO7
#define PIN_WIRE_STOP    21   // GPIO21

// --- Тайминги ---
#define WIRE_HOLD_START_MS   3000  // 3 секунды для старта
#define WIRE_HOLD_STOP_MS    2000  // 2 секунды для стопа
#define CHECK_CONTACT_MS     3000  // Проверка контакта
#define ECG_SAMPLE_RATE_HZ   50   // Частота опроса АЦП
#define ECG_SAMPLE_INTERVAL  (1000000 / ECG_SAMPLE_RATE_HZ)


#define BUFFER_SIZE          20
#endif // CONFIG_H