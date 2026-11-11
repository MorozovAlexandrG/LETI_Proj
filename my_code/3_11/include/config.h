#ifndef CONFIG_H
#define CONFIG_H

// === Пины AD8232 ===
#define PIN_ECG_OUT      2  // Аналоговый выход сигнала
#define PIN_ECG_LO_MINUS 3  // Индикатор отсоединения LO-
#define PIN_ECG_LO_PLUS  4  // Индикатор отсоединения LO+
#define PIN_ECG_SDN      5  // Питание датчика (LOW = включен, HIGH = выключен для экономии)

// === Пин управления ===
#define PIN_BUTTON       6  // Кнопка (подтянута к VCC, нажатие = LOW)

// === Пины SD карты (SPI) ===
#define PIN_SD_CS        10
#define PIN_SD_SCK       8
#define PIN_SD_MISO      9
#define PIN_SD_MOSI      7

// === Настройки записи ===
#define ECG_SAMPLE_INTERVAL 20000  // 20 000 мкс = 20 мс (50 Гц, оптимально для ЭКГ)
#define BUFFER_SIZE      256       // 256 точек в буфере (занимает ~1.5 КБ ОЗУ, безопасно)

// === Настройки WiFi (ЗАПОЛНИТЕ СВОИ!) ===
#define WIFI_SSID        "YOUR_WIFI_NAME"
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"

#endif