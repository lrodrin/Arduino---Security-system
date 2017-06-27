#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <string.h>
#include <NewPing.h>

#define MAXIMUM_DISTANCE 500  //in cm
#define PING_INTERVAL 33  //Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo)
unsigned long pingTimer[2]; //Holds the times when the next ping should happen for each sensor
unsigned int cm[2]; //Where the ping distances are stored
uint8_t currentSensor = 0;  //Keeps track of which sensor is active.

// Sensor object. Each sensor's trigger pin, echo pin, and max distance to ping
NewPing sensor0 = NewPing(3, 5, MAXIMUM_DISTANCE);
NewPing sensor1 = NewPing(6, 9, MAXIMUM_DISTANCE); 

//Global Variables
char ssid[5]; //Our network SSID name
char pass[18];
byte mac[6];  //MAC address
char server[15];
int radioStatus;  //The Wifi radio's status
WiFiClient client;
int estat;
int distance0, distance1;

//estat = 0 -> IDLE
//estat = 1 -> SEND
//estat = 2 -> SLEEP
unsigned long tics;
unsigned long temps;
unsigned long timeConnect;
unsigned long timeSend;
String macaddress;
int range = 0; //Holds the range where sensors detect something 
bool sendCounter = false; //Signal to sent at the counter
bool ackOK = false; //Signal to wait for ack, true when received
bool counted = false; //Signal to know if someone was detected
bool detecta1 = false, detecta0 = false;
int counter = 0; //Global counter
double timer, fd = 0; //Sensor task timer, and fake delay timer (count function)

//Init wifi
void init_wifi() {
  tics = millis();
  strcpy(ssid, "XARD"); 
  strcpy(pass, "PTIN:@2015arduino");
  WiFi.macAddress(mac); //Variable where we store the identifier MAC wifi shield
  strcpy(server, "147.83.159.201");    //Name address for Google (using DNS)
  radioStatus = WL_IDLE_STATUS;
  macaddress = get_mac(mac);
  estat = 0;
}

//Init sensors
void init_sensors() {
  //Set the printer of the queue
  timer = millis();
  fd = millis();
}

//Sensor task: count people
void sensor_task() {   
  int state;  
  if (!counted) { //If none was previously counted or not time to send, keep counting 
  //And if time to send but none counted keep counting
    state = 1;
  }
  else if (counted) { //If someone was counted and is time to send
    state = 2;
  }
  switch (state) {
    case 1:
      count();
      timer = millis();
      break;
    case 2:
      //Trigger send signals
      sendCounter = true;
      ackOK = false;
      counted = false;
      break;
  }
}

//Wifi task: send the counter
void wifi_task() {
  if (estat == 0) {
    idle();
  }
  else if(estat == 1) {
    envia();
  }
  else if ( estat == 2) {
    sleep();
  } 
}

void idle() {
  temps = millis();
  if ((radioStatus != WL_CONNECTED) and (((temps - timeConnect) > 10000) or ((temps-timeConnect) < 0))) {
    temps = millis();
    timeConnect = millis();
    connect_wifi();
  }
  if (radioStatus == WL_CONNECTED) {
    if(sendCounter == true) {
       estat = 1;
    }
  }
  if((sendCounter==false) and ((temps-timeSend) > 300000) or ((temps-timeSend) < 0)) {
    //We verify that it takes 5 minutes and that there are no pending data
    estat = 2;
    WiFi.disconnect();
  }
  temps = millis();
}

//Chechs the 2 sensors for distance
void count() {
  distance0 = sensor0.ping_cm();
  distance1 = sensor1.ping_cm();
  int tempsFD = 1000;
  if(fd == 0 or fd+tempsFD < millis()) {    
      if(distance0 < 50) {        
        //Serial.println("sensor 0: ");
        //Serial.println(distance0);
            
        if(detecta1 and !detecta0) {          
          //Serial.println("people in the room: ");
          //Serial.println(counter); 
          counter++;          
        }         
        detecta0 = true;
        
      }
      else if(distance1 < 50) {        
        //Serial.print("sensor 1:");
        //Serial.println(distance1);
        
        if(detecta0 and !detecta1) {          
          counter--;
          //Serial.print("people in the room:");
          //Serial.println(counter);
          
        }    
        detecta1 = true;    
      }      
      if(detecta0 and detecta1 and distance0 > 50 and distance1 > 50) {
        detecta0 = false;
        detecta1 = false;
        Serial.println("people in the room: ");
        Serial.println(counter);     
        fd = millis();
      }
  }
}

void envia() {
    Serial.println("connected to server");
    //Make an HTTP request
    client.println("GET /"+ macaddress + "/" + counter);
    client.println("host: www.google.com"); //Server name
    client.println("connection: close");
    client.println();
    estat = 0;
    sendCounter = false;
}

void sleep() {
  Serial.println("sleep");
  if (sendCounter) estat = 0;
}

//Display by screen the WiFi MAC
String get_mac(byte mac[]) {   
  String macstr;
  for (int i=5; i >= 0; i--) {
    String temp = String(mac[i], HEX);
    if (temp.length() == 1) temp = String("0" + temp);
    macstr = String(macstr + temp);
    if (i > 0) macstr = String(macstr + ":");
  }
  return macstr;
}

void connect_wifi(){
  Serial.print("attempting to connect to SSID: ");   
  Serial.println(ssid);
  //Connect to WPA/WPA2 network. Change this line if using open or WEP network
  radioStatus = WiFi.begin(ssid, pass);
}

void setup() {
 Serial.begin(9600);
  //init_wifi();
  init_sensors();
}

void loop() {
  if ((millis() - tics) > 10) {
    //wifi_task();
    tics = millis();
  }
  if ((millis() - timer) > 5) {
    sensor_task();
    timer = millis();
  }
}
