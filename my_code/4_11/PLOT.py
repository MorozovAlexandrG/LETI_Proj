import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import time

PORT = '/dev/ttyACM0'
BAUD = 115200

print("Подключение...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    time.sleep(2)
    print("OK")
except:
    print("Ошибка порта!")
    exit()

data = deque(maxlen=500)

fig, ax = plt.subplots(figsize=(12, 5))
line, = ax.plot([], [], 'g-', linewidth=1)
ax.set_title("Сырой сигнал AD8232")
ax.set_xlabel("Время (точки)")
ax.set_ylabel("Значение АЦП")
ax.grid(True, alpha=0.3)

def animate(i):
    while ser.in_waiting > 0:
        try:
            raw = ser.readline().decode('utf-8', errors='ignore').strip()
            if raw.isdigit():
                val = int(raw)
                if 0 <= val <= 4095:
                    data.append(val)
        except:
            pass
    
    if len(data) > 10:
        line.set_data(range(len(data)), list(data))
        ax.set_xlim(0, len(data))
        
        # АВТОМАТИЧЕСКИЙ МАСШТАБ: показываем только область сигнала
        min_val = min(data) - 50
        max_val = max(data) + 50
        # Если размах слишком маленький — расширяем для наглядности
        if max_val - min_val < 200:
            center = (min_val + max_val) / 2
            min_val = center - 200
            max_val = center + 200
        ax.set_ylim(min_val, max_val)
        
    return line,

ani = animation.FuncAnimation(fig, animate, interval=40, blit=False, cache_frame_data=False)

try:
    plt.show()
except KeyboardInterrupt:
    ser.close()