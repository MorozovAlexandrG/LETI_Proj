import serial
import matplotlib.pyplot as plt
import time
import serial.tools.list_ports

# Автопоиск порта
ports = list(serial.tools.list_ports.comports())
PORT = None
for p in ports:
    if "ACM" in p.device or "USB" in p.device:
        PORT = p.device
        break

if not PORT:
    print("Плата не найдена!")
    exit()

print(f"Подключение к {PORT}...")
try:
    ser = serial.Serial(PORT, 115200, timeout=0.1)
    time.sleep(2)
except:
    print("Ошибка: Порт занят! Закрой монитор в VS Code.")
    exit()

print("Готово! Зажми кнопку BOOT на 1 секунду, чтобы выгрузить файл...")

data = []
in_file = False

try:
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if "НАЧАЛО ВЫГРУЗКИ" in line:
                in_file = True
                data = []
                print("\n[ПИТОН]: Читаю файл...")
                continue
            
            if "КОНЕЦ ВЫГРУЗКИ" in line and in_file:
                in_file = False
                print(f"[ПИТОН]: Файл прочитан! Точек: {len(data)}")
                break
            
            if in_file and "," in line:
                parts = line.split(",")
                if len(parts) == 2 and parts[1].strip().isdigit():
                    data.append(int(parts[1].strip()))
                
except KeyboardInterrupt:
    print("\n[ПИТОН]: Остановлено.")

ser.close()

if len(data) > 0:
    print(f"[ПИТОН]: Рисую график из {len(data)} точек...")
    plt.figure(figsize=(12, 5))
    plt.plot(data, 'g-', linewidth=1)
    plt.title("ЭКГ с SD-карты")
    plt.xlabel("Время (точки)")
    plt.ylabel("Значение АЦП")
    plt.grid(True)
    plt.show()
else:
    print("[ПИТОН]: Данные не получены. Ты точно зажал кнопку на 1 секунду?")