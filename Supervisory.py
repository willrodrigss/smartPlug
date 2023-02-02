#importação das bibliotecas necessárias:

#widgets para leitura dos arquivos .ui
from PyQt5 import  uic, QtCore, QtGui, QtWidgets      
from PyQt5.QtGui import QPixmap
from PyQt5.uic import loadUi

import time         #implementar atrasos
import threading    #execução de tarefas "em paralelo"
from paho.mqtt import client as mqtt
import random

app=QtWidgets.QApplication([])      #armazenamento da classe do QtWidgets em uma variável
view=uic.loadUi("view.ui")      #armazenamento e carregamento da tela em .ui

mqttBroker ="broker.mqtt-dashboard.com" 
port = 1883
clientid = "SUPERVISORY"
topic1 = "SMARTPLUG_REALTIMEACTIVATION"


client = mqtt.Client(clientid)
client.connect(mqttBroker, port) 

def main():
    while True:
        #publish
        payload = str(view.level.value())
        client.publish("SMARTPLUG_LEVEL", payload)
        print("Just published " + str(payload) + " to topic POWER LEVEL")
        
        #subscribe
        client.subscribe(topic1)
        client.on_message = on_message
        time.sleep(0.5)
        print(" ")



def on_message(client, userdata, msg):
    realPercentage = (10000-float(msg.payload.decode()))/100
    view.realLevel.setText(str(realPercentage))
    


#execução das aplicações:

threading.Thread(target=main).start()     #execução da função que atualiza os dados em formato de thread ("paralela")
threading.Thread(target=client.loop_start).start()     #execução da função que atualiza os dados em formato de thread ("paralela")



view.show()     #exibição da tela em .ui
app.exec();     #executando aplicação do QWidgets 





    