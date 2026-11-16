#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>

#include "config.h"

// D0..D10 задаёт вариант платы XIAO ESP32-C3.
static const uint8_t ECG_PIN = D0;
static const uint8_t BUTTON_PIN = D1;
static const uint8_t SD_CS = D2;
static const uint8_t SD_SCK = D8;
static const uint8_t SD_MISO = D9;
static const uint8_t SD_MOSI = D10;

static const uint32_t PERIOD_US = 1000000UL / SAMPLE_RATE;
static const uint32_t HOLD_MS = 3000;
static const uint32_t DOUBLE_GAP_MS = 450;
static const uint32_t DEBOUNCE_MS = 40;
static const uint32_t WIFI_TIMEOUT_MS = 15000;
static const char *BOUNDARY = "ecgboundary";

enum Gesture { GESTURE_NONE, GESTURE_DOUBLE, GESTURE_HOLD };

static bool sdReady = false;
static bool recording = false;
static File logFile;
static String currentPath;
static uint32_t nextSampleUs = 0;
static uint32_t sampleIndex = 0;
static uint32_t rowsSinceFlush = 0;

static bool buttonDown = false;
static bool holdFired = false;
static bool waitingSecond = false;
static uint32_t pressStartedMs = 0;
static uint32_t lastReleaseMs = 0;
static uint32_t lastEdgeMs = 0;

static bool beginCard() {
  if (sdReady) {
    return true;
  }
  sdReady = SD.begin(SD_CS, SPI, 1000000);
  if (sdReady) {
    Serial.println("SD: готово");
  }
  return sdReady;
}

static uint32_t takeNextIndex() {
  uint32_t next = 1;
  if (SD.exists("/index.txt")) {
    File in = SD.open("/index.txt", FILE_READ);
    if (in) {
      long value = in.parseInt();
      if (value > 0) {
        next = (uint32_t)value;
      }
      in.close();
    }
  }
  SD.remove("/index.txt");
  File out = SD.open("/index.txt", FILE_WRITE);
  if (out) {
    out.println(next + 1);
    out.close();
  }
  return next;
}

static bool startRecording() {
  if (!beginCard()) {
    Serial.println("SD: нет карты");
    return false;
  }
  uint32_t number = takeNextIndex();
  char path[24];
  snprintf(path, sizeof(path), "/ecg_%04lu.csv", (unsigned long)number);
  logFile = SD.open(path, FILE_WRITE);
  if (!logFile) {
    Serial.println("SD: не открыть файл");
    return false;
  }
  currentPath = path;
  logFile.println("t_ms,ecg");
  logFile.flush();
  sampleIndex = 0;
  nextSampleUs = micros() + PERIOD_US;
  rowsSinceFlush = 0;
  recording = true;
  Serial.print("Запись: ");
  Serial.println(currentPath);
  return true;
}

static void stopRecording() {
  if (!recording) {
    return;
  }
  if (logFile) {
    logFile.flush();
    logFile.close();
  }
  recording = false;
  Serial.print("Стоп: ");
  Serial.println(currentPath);
}

static void sampleIfDue() {
  uint32_t nowUs = micros();
  if ((int32_t)(nowUs - nextSampleUs) < 0) {
    return;
  }
  if ((int32_t)(nowUs - nextSampleUs) >= (int32_t)(PERIOD_US * 2)) {
    nextSampleUs = nowUs + PERIOD_US;
  } else {
    nextSampleUs += PERIOD_US;
  }

  int value = analogRead(ECG_PIN);
  uint32_t tMs = (sampleIndex * 1000UL) / SAMPLE_RATE;
  sampleIndex++;
  logFile.print(tMs);
  logFile.print(',');
  logFile.println(value);
  rowsSinceFlush++;
  if (rowsSinceFlush >= SAMPLE_RATE) {
    logFile.flush();
    rowsSinceFlush = 0;
  }
}

static Gesture pollGesture() {
  bool down = digitalRead(BUTTON_PIN) == LOW;
  uint32_t now = millis();

  if (down != buttonDown && (now - lastEdgeMs) < DEBOUNCE_MS) {
    down = buttonDown;
  }

  if (down && !buttonDown) {
    buttonDown = true;
    holdFired = false;
    pressStartedMs = now;
    lastEdgeMs = now;
  }

  if (down && buttonDown && !holdFired && (now - pressStartedMs) >= HOLD_MS) {
    holdFired = true;
    waitingSecond = false;
    lastEdgeMs = now;
    return GESTURE_HOLD;
  }

  if (!down && buttonDown) {
    uint32_t held = now - pressStartedMs;
    buttonDown = false;
    lastEdgeMs = now;
    if (!holdFired && held < HOLD_MS) {
      if (waitingSecond && (now - lastReleaseMs) <= DOUBLE_GAP_MS) {
        waitingSecond = false;
        return GESTURE_DOUBLE;
      }
      waitingSecond = true;
      lastReleaseMs = now;
    }
  }

  if (waitingSecond && (now - lastReleaseMs) > DOUBLE_GAP_MS) {
    waitingSecond = false;
  }
  return GESTURE_NONE;
}

static String baseName(const String &path) {
  int slash = path.lastIndexOf('/');
  if (slash >= 0) {
    return path.substring(slash + 1);
  }
  return path;
}

static bool writeAll(WiFiClient &client, const uint8_t *data, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    int n = client.write(data + sent, len - sent);
    if (n <= 0) {
      return false;
    }
    sent += (size_t)n;
    yield();
  }
  return true;
}

static bool writeString(WiFiClient &client, const String &data) {
  return writeAll(client, (const uint8_t *)data.c_str(), data.length());
}

static String multipartHead(const String &filename) {
  String head;
  head.reserve(320);
  auto addField = [&](const char *name, const String &value) {
    head += "--";
    head += BOUNDARY;
    head += "\r\nContent-Disposition: form-data; name=\"";
    head += name;
    head += "\"\r\n\r\n";
    head += value;
    head += "\r\n";
  };
  addField("patient_name", PATIENT_NAME);
  addField("device_id", DEVICE_ID);
  addField("sample_rate", String(SAMPLE_RATE));
  head += "--";
  head += BOUNDARY;
  head += "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"";
  head += filename;
  head += "\"\r\nContent-Type: text/csv\r\n\r\n";
  return head;
}

static bool uploadOne(const String &path, const String &filename) {
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file) {
    Serial.print("SD: не прочитать ");
    Serial.println(path);
    return false;
  }

  String head = multipartHead(filename);
  String tail = String("\r\n--") + BOUNDARY + "--\r\n";
  uint32_t contentLength = head.length() + (uint32_t)file.size() + tail.length();

  WiFiClient client;
  client.setTimeout(20000);
  if (!client.connect(SERVER_HOST, SERVER_PORT)) {
    Serial.println("Сервер: нет соединения");
    file.close();
    return false;
  }
  client.setNoDelay(true);

  String request = String("POST /api/upload HTTP/1.1\r\nHost: ") + SERVER_HOST + ":" +
                   SERVER_PORT + "\r\nX-Device-Token: " + DEVICE_TOKEN +
                   "\r\nContent-Type: multipart/form-data; boundary=" + BOUNDARY +
                   "\r\nContent-Length: " + contentLength +
                   "\r\nConnection: close\r\n\r\n";
  if (!writeString(client, request) || !writeString(client, head)) {
    file.close();
    client.stop();
    return false;
  }

  uint8_t buffer[512];
  while (file.available()) {
    int n = file.read(buffer, sizeof(buffer));
    if (n < 0) {
      file.close();
      client.stop();
      return false;
    }
    if (n == 0) {
      break;
    }
    if (!writeAll(client, buffer, (size_t)n)) {
      file.close();
      client.stop();
      return false;
    }
  }
  file.close();
  if (!writeString(client, tail)) {
    client.stop();
    return false;
  }
  client.flush();

  uint32_t waitStarted = millis();
  while (client.connected() && !client.available() && millis() - waitStarted < 20000) {
    delay(10);
  }
  String status = client.readStringUntil('\n');
  status.trim();
  client.stop();
  Serial.println(status);
  return status.startsWith("HTTP/1.1 200") || status.startsWith("HTTP/1.0 200");
}

static void markSent(const String &path) {
  String marker = path + ".sent";
  File flag = SD.open(marker.c_str(), FILE_WRITE);
  if (flag) {
    flag.close();
  }
}

static bool isPendingCsv(const String &name) {
  if (!name.startsWith("ecg_") || !name.endsWith(".csv")) {
    return false;
  }
  String marker = String("/") + name + ".sent";
  return !SD.exists(marker.c_str());
}

static bool connectWifi() {
  Serial.print("Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < WIFI_TIMEOUT_MS) {
    delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi: нет сети");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }
  Serial.print("Wi-Fi: ");
  Serial.println(WiFi.localIP());
  return true;
}

static void uploadPending() {
  if (!beginCard()) {
    Serial.println("SD: нет карты");
    return;
  }
  if (!connectWifi()) {
    return;
  }

  File dir = SD.open("/");
  if (!dir) {
    Serial.println("SD: не открыть корень");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return;
  }

  File entry;
  while ((entry = dir.openNextFile())) {
    String name = baseName(String(entry.name()));
    entry.close();
    if (!isPendingCsv(name)) {
      continue;
    }
    String path = String("/") + name;
    Serial.print("Отправка: ");
    Serial.println(path);
    if (uploadOne(path, name)) {
      markSent(path);
      Serial.println("Отправлено");
    } else {
      Serial.println("Ошибка отправки");
    }
  }
  dir.close();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  Serial.println("Холтер: двойное нажатие — запись, удержание 3 с — отправка");
  beginCard();
}

void loop() {
  Gesture gesture = pollGesture();
  if (gesture == GESTURE_DOUBLE) {
    if (recording) {
      stopRecording();
    } else {
      startRecording();
    }
  } else if (gesture == GESTURE_HOLD) {
    if (recording) {
      stopRecording();
    }
    uploadPending();
  }

  if (recording) {
    sampleIfDue();
  }
}
