//libraries
#include <WiFi.h>
#include <PubSubClient.h>

//naming pins
#define trigger 12
#define zeroCross 22

//limits to the delay detection
unsigned long minTime = 600;
unsigned long maxTime = 9600;

//to stop interruptions
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

//timer
hw_timer_t *timer = NULL;

//store the time delay to pulse the gate
uint64_t timeActivation = 0;

//store the percentage of power intended and the previous
volatile float level = 0;
volatile float previousLevel = 0;

//for calculus (power, conspumtion...)
uint64_t crossingTime = 0; //register the moment of the zero crossing
uint64_t pinHighTime = 0; //register the moment of rising edge of TRIAC
uint64_t timeOff = 0;  //to calculate the real waiting time

//--------------------WiFi attributes--------------------
/*
const char *SSID = "Rep Bairro Nobre";
const char *PASSWORD =  "cheesenobre2022";
*/

/*
const char* SSID = "agents";
const char* PASSWORD =  "QgC9O8VucAByqvVu5Rruv1zdpqM66cd23KG4ElV7vZiJND580bzYvaHqz5k07G2";
*/

///*
const char* SSID = "will";
const char* PASSWORD =  "12ab34cd";
//*/

WiFiClient Wificlient;   //storage the data of the connected client 
//------------------------------------------------------

//--------------------MQTT attributes--------------------
const char* BROKER_MQTT = "broker.mqtt-dashboard.com"; 
int BROKER_PORT = 1883;

#define ID_MQTT "PLUG" //unique for each device
#define TOPIC_SUBSCRIBER "SMARTPLUG_LEVEL"  //same between the publisher and subscriber
#define TOPIC_PUBLISHER "SMARTPLUG_REALTIMEACTIVATION"
PubSubClient MQTT(Wificlient);
//------------------------------------------------------

//----------------------functions-----------------------
void connectWiFi();
void connectMQTT();
void keepConnections();
void receivePackage(char* topic, byte* payload, unsigned int length);
void sendPackage();
//------------------------------------------------------

//--------------------interruptions---------------------
//ISR to regist the zero-crossing
void IRAM_ATTR ISR_zeroCross()  {
  //if(currentBrightness == IDLE) return;
  portENTER_CRITICAL_ISR(&mux); // turn off interruptions for a while
    crossingTime = micros();  //catch this moment
    setTimer(timeActivation, &ISR_turnPinHigh); // define the time delay for triggering
  portEXIT_CRITICAL_ISR(&mux); // turn on the interruptions
}

//set the timer parameters to rising (ISR_turnPinHigh) and pulse duration (ISR_turnPinLow)
ICACHE_RAM_ATTR void setTimer(uint64_t timeConfig, void (*ISR)(void)){ 
  //portENTER_CRITICAL_ISR(&mux);
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, ISR, true);
    timerAlarmWrite(timer, timeConfig, false);
    timerAlarmEnable(timer);
  //portEXIT_CRITICAL_ISR(&mux);
}

//rising trigger pulse
ICACHE_RAM_ATTR void ISR_turnPinHigh(){ 
  portENTER_CRITICAL_ISR(&mux);  
    digitalWrite(trigger, HIGH);
    pinHighTime = micros(); //catch this moment
    timeOff = pinHighTime-crossingTime;
    setTimer(70, &ISR_turnPinLow);
  portEXIT_CRITICAL_ISR(&mux); 
}

//falling trigger pulse
ICACHE_RAM_ATTR void ISR_turnPinLow(){
  portENTER_CRITICAL_ISR(&mux);
    digitalWrite(trigger, LOW);
  portEXIT_CRITICAL_ISR(&mux);
}
//------------------------------------------------------

//--------------mainly arduino functions----------------
void setup() {
  // put your setup code here, to run once:
  pinMode(zeroCross, INPUT_PULLUP);  //detect the zero cross
  pinMode(trigger, OUTPUT); //pin to pulse the gate
  Serial.begin(115200); //speed rate

  digitalWrite(trigger, LOW);   //start low before calculate
  //attachInterrupt(activactor of interruption, function to be run in the moment of interruption, mode)
  
  WiFi.begin(SSID, PASSWORD); 
  connectWiFi();
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(receivePackage);

  attachInterrupt(digitalPinToInterrupt(zeroCross), ISR_zeroCross, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  keepConnections();
  sendPackage();
  MQTT.loop();

  timeActivation = map(level, 0, 100, maxTime, minTime);
  Serial.println(level);
}
//------------------------------------------------------

//-----------Wi-Fi and MQTT implementations-------------
void connectWiFi(){
    if (WiFi.status() == WL_CONNECTED) {
     return;
    }
  
    while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
    }
  
    Serial.println(" ");
    Serial.print("Connected to WiFi ");
    Serial.print(SSID);
    Serial.print(" | IP Address: ");
    Serial.println(WiFi.localIP());
}

void connectMQTT(){
  while (!MQTT.connected()){
    Serial.print("Connecting to Broker ");
    Serial.println(BROKER_MQTT);

    if(MQTT.connect(ID_MQTT)){
      Serial.println("Connection to Broker successfully!");
      MQTT.subscribe(TOPIC_SUBSCRIBER);
    }

    else{
      Serial.println("No connection found. Trying to reconnect...");
      delay(1000);
    }
  }
}

void keepConnections(){
  if(!MQTT.connected()){
    connectMQTT();
  }
  connectWiFi();
}

void receivePackage(char* topic, byte* payload, unsigned int length){
  String msg; //string to store the package message

  for(int i = 0; i < length; i++){
    char c = (char)payload[i];
    msg += c;
  }

  level = msg.toDouble();
}

void sendPackage(){
  char dif[5]={0};

  sprintf(dif,"%.2d", timeOff);    //convert to string
  
  MQTT.publish(TOPIC_PUBLISHER, dif);
}

 

 


