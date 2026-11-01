import serial
import matplotlib.pyplot as plt
from collections import deque

PORT = '/dev/ttyACM0'
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=0.1)
data = deque(maxlen=500)

plt.ion() 
fig, ax = plt.subplots()
line, = ax.plot([], [], 'g-')

ax.set_ylim(900, 950) 
ax.set_title("ЭКГ в реальном времени (Y: 750-1050)")

try:
    while True:
        while ser.in_waiting > 0:
            try:
                line_str = ser.readline().decode('utf-8').strip()
                if line_str.isdigit():
                    data.append(int(line_str))
            except:
                pass
        
        if len(data) > 0:
            line.set_data(range(len(data)), list(data))
            ax.set_xlim(0, len(data))
            fig.canvas.draw()
            fig.canvas.flush_events()
            
except KeyboardInterrupt:
    print("Закрыто")