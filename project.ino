#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h> 
#include <LiquidCrystal_I2C.h>
#include <DallasTemperature.h>

#define mqtt_port 1883
#define temp_topic "temperatureC";
#define dis_topic "jarak";
#define rtc_topic "rtc_time";
#define rtcdate_topic "rtc_date";
#define buzzer 16

// Update these with values suitable for your network.
const char* ssid = "Nasi Gratisan";
const char* password = "mbayarsek";
const char* mqtt_server = "test.mosquitto.org";
const char* servoTopic = "servo";

//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); 

//Servo Settings
Servo servo;


//ultrasonic
const int trigPin = 13;   
const int echoPin = 12;   
long duration;  
int distance;  

// GPIO where the DS18B20 is connected to
#define oneWireBus 14
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
// Temperature value
int temp;
unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

//RTC
RTC_DS3231 RTC;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

long lastMsg = 0;
char msg[50];
int value = 0;

const int lamp = 2;

void setup_wifi() {
 delay(10);
 // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect() {
 // Loop until we're reconnected
 while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    //Once connected, publish an announcement...
    // ... and resubscribe
    client.subscribe("ledcontrol1");
    client.subscribe("servo");
 } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
 }
}
// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* payload, unsigned int length) {
    String string;
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  
  for (int i = 0; i < length; i++) {
  Serial.print(payload[i]);
}   
   if (payload[0] == 49)digitalWrite(lamp, HIGH);
   else if (payload [0]==48)digitalWrite(lamp, LOW);

   if (payload[0]==50) {
    servo.write(90);
    delay(1000);
    servo.write(0);
   }
   Serial.println();
 }

void setup() {
 Serial.begin(115200); // Set serial port speed
 sensors.begin();
 lcd.init();
 lcd.backlight();
#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif
 if (! RTC.begin()) {
    Serial.println("RTC tidak terbaca");
    while (1);
  }
 if (RTC.lostPower()) {
    //atur waktu sesuai waktu pada komputer
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //atur waktu secara manual
    // January 21, 2019 jam 10:30:00
    //RTC.adjust(DateTime(2023, 7, 31, 11, 54 , 0));
  }
 pinMode(lamp, OUTPUT);
 pinMode(buzzer, OUTPUT);
 servo.attach(0);
 pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output  
 pinMode(echoPin, INPUT); // Sets the echoPin as an Input
 Serial.setTimeout(500);// Set time out for
 setup_wifi();
 client.setServer(mqtt_server, mqtt_port);
 client.setCallback(callback);
 //reconnect();
}

void loop() {
 
 if (!client.connected()) {
  reconnect();
   }
 client.loop();

 // Clears the trigPin  
 digitalWrite(trigPin, LOW);  
 delayMicroseconds(2);
     
 // Sets the trigPin on HIGH state for 10 micro seconds  
 digitalWrite(trigPin, HIGH);  
 delayMicroseconds(10);  
 digitalWrite(trigPin, LOW);
     
 // Reads the echoPin, returns the sound wave travel time in microseconds  
 duration = pulseIn(echoPin, HIGH);
     
 // Calculating the distance  
 distance= duration*0.034/2;
     
 // Prints the distance on the Serial Monitor
 char disString[8];
 dtostrf(distance, 1, 2, disString);  
 Serial.print(" Distance: ");  
 Serial.println(distance);  
 client.publish("jarak", disString);

 unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    // New temperature readings
    sensors.requestTemperatures(); 
    // Temperature in Celsius degrees
    temp = sensors.getTempCByIndex(0);
    }
    char tempString[9];
    dtostrf(temp, 3, 4, tempString);  
    Serial.print(" Suhu: ");  
    Serial.println(temp);  
    client.publish("temperatureC", tempString);
  
 lcd.setCursor(0,0);
 lcd.print("Dist:");
 lcd.print(distance);
 lcd.print("CM");
 //delay(1000);
 lcd.setCursor(10,0);
 lcd.print("T:");
 lcd.print(temp);
 lcd.print((char)223);
 lcd.print("C");

 char times[12];
 char dates[10];
 DateTime now = RTC.now();
 Serial.print(now.day(), DEC);
 Serial.print('/');
 Serial.print(now.month(), DEC);
 Serial.print('/');
 Serial.print(now.year(), DEC);
 Serial.println();
 Serial.print(now.hour(), DEC);
 Serial.print(':');
 Serial.print(now.minute(), DEC);
 Serial.print(':');
 Serial.print(now.second(), DEC);
 Serial.println();

 
 sprintf(times,"%02d:%02d:%02d", now.hour(), now.minute(), now.second());
 sprintf(dates,"%02d/%02d/%02d", now.day(), now.month(), now.year()); 
 client.publish("rtc_time",times);
 client.publish("rtc_date", dates);

 //ATUR BUZZER
 if (distance >= 20){
  digitalWrite (buzzer, HIGH);
 }
 else if (temp < 25){
  digitalWrite (buzzer, HIGH);
 }
 else if (temp >= 30){
  digitalWrite (buzzer, HIGH);
 }
 else{
  digitalWrite (buzzer, LOW);
 }


 //ATUR SERVO SESUAI WAKTU
 if ((now.hour()==7)&&(now.minute()==0)&&(now.second()==0)){
    servo.write(90);
    delay(1000);
    servo.write(0);
   }

 if ((now.hour()==12)&&(now.minute()==0)&&(now.second()==0)){
    servo.write(90);
    delay(1000);
    servo.write(0);
   }

 if ((now.hour()==17)&&(now.minute()==0)&&(now.second()==0)){
    servo.write(90);
    delay(1000);
    servo.write(0);
   }
 
 lcd.setCursor(0, 1);
 lcd.print("Time: "); 
 lcd.print(now.hour(), DEC);
 lcd.print(":");
 lcd.print(now.minute(), DEC);
 lcd.print(":");
 lcd.print(now.second(), DEC);
 delay(1000); 
 lcd.clear();
}