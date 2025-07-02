#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <PubSubClient.h>

LiquidCrystal lcd(0, 1, 2, 3, 4, 5);

#define DHTPIN 8
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
const int ledPin = 9; // LED pin
const int ldrPin = 28; // LDR pin
const int fanRelayPin = 22; // Fan relay pin
const int acPin = 17; // AC relay pin

int ledBrightness = 0;
bool isFanOn = false;
bool isACOn = false;

// MQTT Credentials
const char* ssid = "Wokwi-GUEST"; // Setting your AP SSID
const char* password = ""; // Setting your AP PSK
const char* mqttServer = "broker.hivemq.com";

const char* clientID = "pi-Wokwi"; //Random
const char* topic = "TempData"; 

// Parameters for using non-blocking delay
unsigned long previousMillis = 0;
const long interval = 1000;
String msgStr = "";      // MQTT message buffer

// Setting up WiFi and MQTT client
WiFiClient piClient;
PubSubClient client(piClient);

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(clientID)) {
      Serial.println("MQTT connected");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);  // wait 5sec and retry
    }
  }
}

// Subscribe callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  String data = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println();
  Serial.print("Message size: ");
  Serial.println(length);
  Serial.println();
  Serial.println("-----------------------");
  Serial.println(data);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(ldrPin, INPUT);
  pinMode(fanRelayPin, OUTPUT);
  pinMode(acPin, OUTPUT);
  
  dht.begin();
  lcd.begin(20, 4);
  lcd.setCursor(2, 2);
  lcd.print("LED");
  lcd.setCursor(8, 2);
  lcd.print("FAN");
  lcd.setCursor(14, 2);
  lcd.print("AC");
  
  setup_wifi();
  client.setServer(mqttServer, 1883); // Setting MQTT server
  client.setCallback(callback); // Define function which will be called when a message is received
}

void loop() {
  nodeRed();
  readSensors();
  controlDevices();
  displayStatus();
  delay(1);
}

void nodeRed(){
  if (!client.connected()) { // If client is not connected
    reconnect(); // Try to reconnect
  }
  client.loop();
  unsigned long currentMillis = millis(); // Read current time
  if (currentMillis - previousMillis >= interval) { // If current time - last time > 5 sec
    previousMillis = currentMillis;
  }
  int temp = dht.readTemperature();
  Serial.print(F("Temperature: "));
  Serial.print(temp);
  Serial.println(F("°C"));
  msgStr = String(temp);
  byte arrSize = msgStr.length() + 1;
  char msg[arrSize];
  Serial.print("PUBLISH DATA: ");
  Serial.println(msgStr);
  msgStr.toCharArray(msg, arrSize);
  client.publish(topic, msg);
  msgStr = "";
}

void readSensors() {
  // Reading LDR value
  int ldrValue = analogRead(ldrPin);
  ledBrightness = map(ldrValue, 170, 1023, 0, 255);
  int ledPercentage = map(ledBrightness, 0, 255, 0, 100);
  if (ledPercentage<0){
    ledPercentage=0;
  }
  // Reading temperature sensor value
  int tempValue = dht.readTemperature();
  lcd.setCursor(2,3);
  lcd.print(ledPercentage);
  lcd.print(" %");
  lcd.setCursor(0,0);
  lcd.print("TEMP : ");
  lcd.print(tempValue);
  lcd.print(" C");
  
  if (tempValue >= 30) { // Threshold temperature
    isFanOn = false;
    isACOn = true;
  } else if (tempValue < 30 && tempValue >= 20) {
    isFanOn = true;
    isACOn = false;
  } else if (tempValue < 20) {
    isFanOn = false;
    isACOn = false;
  }
}

void controlDevices() {
  analogWrite(ledPin, ledBrightness);
  digitalWrite(fanRelayPin, isFanOn ? HIGH : LOW);
  digitalWrite(acPin, isACOn ? HIGH : LOW);
}

void displayStatus() {
  // Displaying status on LCD
  lcd.setCursor(8, 3);
  lcd.print(isFanOn ? "On " : "Off");
  lcd.setCursor(14, 3);
  lcd.print(isACOn ? "On " : "Off");
}