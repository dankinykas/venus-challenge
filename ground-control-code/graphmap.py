import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import time
import csv


# MQTT settings
broker_address = "mqtt.ics.ele.tue.nl"
topic = "/pynqbridge/32/recv" 

username = "Student63"
password = "aiQu5ail" 

# Matplotlib settings
fig, ax = plt.subplots()
scat = ax.scatter([], [])
ax.grid()


plt.xlabel('X')
plt.ylabel('Y')
plt.title('Venus surface')

x = []
y = []
colour = []
size = []



with open('data.csv', 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(['x','y','colour','size'])
    

# MQTT on_connect callback
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe(topic)

# MQTT on_message callback
def on_message(client, userdata, msg):

    raw = msg.payload.decode("utf-8")
    # print(data)
    data = raw.split('_')
    
    x.append(float(data[0]))
    y.append(float(data[1]))
    colour.append(data[2])
    if data[3] == 'l':
        size.append(float(40))
    else:
        size.append(float(20))
    print(x, y, colour, size)

    with open('data.csv', 'a') as f:
        writer = csv.writer(f)
        writer.writerow([data[0], data[1], data[2], data[3]])

    ax.set_xlim(auto=True)
    ax.set_ylim(auto=True)
    ax.scatter(x, y, s=size, c=colour)
    plt.draw()


# Initialize MQTT client
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(broker_address, 1883, 60)
client.username_pw_set(username, password)


# Start the MQTT loop
client.loop_start()
time.sleep(4)
client.subscribe(topic)

# Display the plot
plt.show()

# Disconnect MQTT client when program ends
client.loop_stop()
client.disconnect()
