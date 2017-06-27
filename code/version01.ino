#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <string.h>
#include <NewPing.h>

#define MAXIMUM_DISTANCE 500// in cm
#define PING_INTERVAL 33// Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).
unsigned long pingTimer[2];// Holds the times when the next ping should happen for each sensor.
unsigned int cm[2];// Where the ping distances are stored.
uint8_t currentSensor = 0;// Keeps track of which sensor is active.

// Sensor object. Each sensor's trigger pin, echo pin, and max distance to ping.
NewPing sensor0 = NewPing(3, 5, MAXIMUM_DISTANCE);
NewPing sensor1 = NewPing(6, 9, MAXIMUM_DISTANCE); 

//Global Variables
char ssid[5];     //  your network SSID (name)
char pass[18];
byte mac[6];                      //MAC address
char server[15];
int status;     // the Wifi radio's status
WiFiClient client;
int estat;
int distance0, distance1;

// estat = 0 -> IDLE
// estat = 1 -> send
// estat = 2 -> SLEEP
unsigned long tics;
unsigned long time;
unsigned long timeConect;
unsigned long timeSend;
String macaddress;
int range = 0; //holds the range where you can detect something 
bool send_counter = false; //signal to sent the counter
bool ack_ok = false; //signal to wait for ack //true when received
bool counted = false; //signal to know if someone was detected
bool detecta1 = false, detecta0 = false; //
//global counter
int counter = 0;
//timers
double timer,fd = 0; //sensor_task timer, and fake deylay timer (count function)
double main_timer; //loop timer

//Init functions
void init_wifi(){
  //Serial.println("Setup!!");
  tics= millis();
  strcpy(ssid, "XARD"); 
  strcpy(pass, "PTIN:@2015arduino");
  WiFi.macAddress(mac);        // variable con guardarem l'identificador MAC del wifishield.
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
  strcpy(server, "147.83.159.201");    // name address for Google (using DNS)
  status = WL_IDLE_STATUS;
  macaddress = getmac(mac);
  estat = 0;
}

void Init_Sensors() {
  //Serial.begin (9600);
  // Set the printer of the queue.
  Calibration();
  timer= millis();
  fd = millis();
}

//Loop Functions
void sensor_task() {
  
  Serial.println("Holaaaaaaaaaaaaaaaaa sensor task");
  
  int state;
  
  if (!counted){ //if none was previously counted or not time to send keep counting //so if time to send but none counted keep counting
    state=1;
  }
  else if (counted){ //if someone was counted and is time to send
    state=2;
  }
  switch (state){
    case 1:
      Serial.println("/////////////////////////////////////////////////////Contar/////////////////////////////////////////////////////");
      Count();
      timer = millis();
      break;
    case 2:
      //Serial.println("/////////////////////////////////////////////////////Enviar/////////////////////////////////////////////////////");
      //trigger send signals
      send_counter = true;
      ack_ok = false;
      counted = false;
      break;
  }
}

void wifi_task(){
  if (estat == 0){
    idle();
  }else if(estat == 1){
    envia();
  }else if ( estat ==2){
    sleep();
  } 
}

//Idle functions
void idle(){
  //Serial.println("iddle");
  time = millis();
  if ((status != WL_CONNECTED) and (((time - timeConect) > 10000) or ((time-timeConect) < 0))) {
    time = millis();
    timeConect = millis();
    conectWifi();
  }
  if (status == WL_CONNECTED) {
    if(send_counter == true){
       estat = 1;
    }
  }
  if((send_counter==false) and ((time-timeSend) > 300000) or ((time-timeSend) < 0)){//comprovem que pasin 5 minuts y que no hi hagin dades pendents
    estat = 2;
    WiFi.disconnect(); //desconectem del wifi
  }
  time = millis();
}

void Count() {//chechs the 2 sensors for distance
  distance0 = sensor0.ping_cm();
  distance1 = sensor1.ping_cm();
  int temps_fd = 1000;
  //Serial.print("HOSTIAA PUTA YA: ");
  //Serial.println(seg);
  if(fd == 0 or fd+temps_fd < millis()){
    
      if(distance0 < 50) {   
        
        Serial.println("detected sensor 0: ");
        Serial.println(distance0);
    
        if(detecta1 and !detecta0) {
          
          Serial.println("people in the room: ");
          Serial.println(counter); 
          counter++;
          
        }
         
        detecta0 = true;
        
      }
      else if(distance1 < 50) {
        
        Serial.print("detected sensor 1:");
        Serial.println(distance1);
        if(detecta0 and !detecta1) {
          
          counter--;
          Serial.print("people in the room:");
          Serial.println(counter);
          
        }    
        detecta1 = true;    
      }
      
      if(detecta0 and detecta1 and distance0 > 50 and distance1 > 50) {
        detecta0 = false;
        detecta1 = false;
        Serial.println("people in the room: ");
        Serial.println(counter);     
        fd = millis();
       // delay(1000);
      }
  }
}

//State functions
void envia(){
      Serial.println("connected to server");
      // Make a HTTP request:
      client.println("GET /"+macaddress+"/"+counter);
      client.println("Host: www.google.com");//name of server
      client.println("Connection: close");
      client.println();
    estat = 0;
    send_counter = false;
}

void sleep(){
  Serial.println("in sleep....");
  if (send_counter) estat=0;
  //if (status != WL_CONNECTED) {
    //time = millis();
    //timeConect = millis();
    //conectWifi();
  //}
}

//Auxiliar functions
String getmac(byte mac[]){ 
  // Mostra per pantalla la MAC de la WiFi:
  String macstr;
  for (int i=5; i >= 0; i--) {
    String temp = String(mac[i], HEX);
    if (temp.length() == 1) temp = String("0" + temp);
    macstr = String(macstr + temp);
    if (i > 0) macstr = String(macstr + ":");
  }
  return macstr;
}

void conectWifi(){
   Serial.print("Attempting to connect to SSID: ");
   
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
}

void Calibration() {
  ///***///set range from 0 to max distande = range
}

void setup() {
 Serial.begin(9600);
  //init_wifi();
  Init_Sensors();
  
  /*Serial.print(ssid);
  Serial.print(pass);
  Serial.print(server);
  Serial.print(estat);*/
}

void loop() {
  if ((millis() - tics) > 10){
    //wifi_task();
    tics= millis();
  }
  if ((millis() - timer) > 5){
    sensor_task();
    timer= millis();
  }
}
