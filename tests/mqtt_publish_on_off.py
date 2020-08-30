import paho.mqtt.client as mqtt
import time

def on_connect(client, userdata, flags, rc):
    print("connected with result code "+str(rc))

client = mqtt.Client()
client.on_connect = on_connect


client.connect("192.168.2.226", 1883, 60)

client.loop_start()

while(True):
    client.publish("home1/noahsbedroom/device/lightshow", "on")
    time.sleep(30)
    client.publish("home1/noahsbedroom/device/lightshow", "off")
    time.sleep(45)

client.loop_stop()
