#include <Arduino.h>
#include <Util.h>
#include <WiFi.h>
#include <time.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>

//SETTINGS
//pins
#define SENSOR_PIN 34
#define VENTIL_PIN 23

//general
#define SAMPLES 1000

//times
#define FLUSH_TIME 7*1000
#define AFTER_FLUSH 5*1000
#define TRIGGER_TIME 5*1000
#define AUTO_TRIGGER 12*60*60*1000

//trigger values
#define AVERAGE 2672
#define MIN_NONE (AVERAGE-156)
#define MAX_NONE (AVERAGE+108)

#define TRIGGER_LOW 2100
#define TRIGGER_HIGH 3000

//MACROS
#define NONE (value > MIN_NONE && value < MAX_NONE)
#define TRIGGER (value < TRIGGER_LOW || value > TRIGGER_HIGH)
#define TRIGGERED (triggered==0 ? TRIGGER : (!NONE))

#define AUTO_FLUSH (millis()-untriggered > AUTO_TRIGGER)
#define TRIGGER_FLUSH (millis()-triggered > TRIGGER_TIME)
#define FLUSH (triggered!=0 ? TRIGGER_FLUSH : AUTO_FLUSH)

#define GET_VALUE (modal([](){return analogRead(SENSOR_PIN);}, SAMPLES))

const char* ssid     = "NetFrame";
const char* password = "87934hzft9oeu4389nv8o437893hf978";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

unsigned long triggered = 0;
unsigned long untriggered = millis();

AsyncWebServer server(80);

bool printValues = false;
bool printStates = true;

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  // WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println("Received Data: "+d);
  delay(5);
  if(d.equals("values")) {
    printValues = !printValues;
    WebSerial.println("printValues: "+(String)(printValues?"true":"false"));
  }
  if(d.equals("states")) {
    printStates = !printStates;
    WebSerial.println("printStates: "+(String)(printStates?"true":"false"));
  }
}

bool checkSensor() {
  short value = GET_VALUE;
  if(TRIGGERED)
    triggered = millis();
  if(Serial && printValues)
    Serial.println(value);
  if(WiFi.status()==WL_CONNECTED && printValues && WebSerial.available())
    WebSerial.println(String(millis())+" "+String(value));
  return FLUSH;
}

void openVentil() {
  if(Serial && printStates)
    Serial.println(F("triggered"));
  if(WiFi.status()==WL_CONNECTED && printStates && WebSerial.available())
    WebSerial.println(F("triggered"));
  digitalWrite(VENTIL_PIN, LOW);
  delay(FLUSH_TIME);
  digitalWrite(VENTIL_PIN, HIGH);
  untriggered = millis();
  while(millis()-untriggered < AFTER_FLUSH) {
    short value = GET_VALUE;
    if(TRIGGER)
      untriggered = millis();
  }
  triggered = 0;
  untriggered = millis();
  if(Serial && printStates)
    Serial.println(F("untriggered"));
  if(WiFi.status()==WL_CONNECTED && printStates && WebSerial.available())
    WebSerial.println(F("untriggered"));
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println(F("Failed to obtain time"));
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Serial started");
  pinMode(VENTIL_PIN, OUTPUT);
  digitalWrite(VENTIL_PIN, HIGH);
  
  WiFi.mode(WIFI_STA); 
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // printLocalTime();
  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);
  server.begin();
}    

void loop() {
  if(checkSensor())
    openVentil();
}