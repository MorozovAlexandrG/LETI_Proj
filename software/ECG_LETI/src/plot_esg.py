import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# === НАСТРОЙКИ ===
# Укажите ваш порт! (Windows: 'COM3', macOS/Linux: '/dev/ttyACM0' или '/dev/cu.usbmodem...')
PORT = '/dev/ttyACM1'  
BAUDRATE = 115200
# =================

try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    ser.setDTR(True)
    ser.setRTS(True)
    print(f"Подключено к {PORT}")
except serial.SerialException:
    print(f"Ошибка: не удалось открыть порт {PORT}. Проверьте номер порта в коде.")
    exit()

data = []
max_points = 300  # Сколько точек показывать на экране

fig, ax = plt.subplots(figsize=(10, 4))
line, = ax.plot([], color='green', linewidth=1.5)
ax.set_ylim(0, 4096)  # Диапазон 12-битного АЦП ESP32-C3
ax.set_xlim(0, max_points)
ax.set_title("ECG AD8232 (XIAO ESP32-C3)")
ax.grid(True)

def animate(i):
    try:
        # Читаем строку из последовательного порта
        line_str = ser.readline().decode('utf-8', errors='ignore').strip()
        
        # Если это валидное число, добавляем его в массив
        if line_str.isdigit():
            data.append(int(line_str))
            
            # Оставляем только последние max_points значений
            if len(data) > max_points:
                data.pop(0)
            
            # Обновляем данные графика
            line.set_ydata(data)
            line.set_xdata(range(len(data)))
            
    except Exception:
        pass
    return line,

# Запуск анимации (интервал ~20 мс = 50 кадров в секунду)
ani = animation.FuncAnimation(fig, animate, interval=20, blit=False, cache_frame_data=False)

plt.show()

# Закрытие порта при закрытии окна
ser.close()