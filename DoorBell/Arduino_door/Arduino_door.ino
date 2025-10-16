#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WIFI config
const char* ssid = "Fablab_Torino";
const char* password = "Fablab.Torino!";

// MQTT config
const char* mqtt_server = "172.26.34.167";
const char* mqtt_topic_door1 = "Door1_topic";
const char* mqtt_username = "";
const char* mqtt_password = "";

// Network objects
WiFiClient espClient;
PubSubClient client(espClient);

// LED and relay config
const int ledPinDoor1 = 13;
bool ledStateDoor1 = false;
unsigned long ledOnTimeDoor1 = 0;

const int relayPin = 4;
unsigned long relayOnTime = 0;

// Creating the method prototipes before the setup and loop to make the code more stable
void connectToWiFi();
void connectToMQTT();
void controlLED(int pin, bool& state, unsigned long& onTime, unsigned long duration);
void triggerRelay(unsigned long duration = 3000);
void publishJson(const char* topic, StaticJsonDocument<200>& doc);
void sendDoorbellMessage(const char* doorName);
void sendNotificationToPublisher(const char* message);

//Setting up the wifi, mqtt, callback, LED and relay
void setup() {
  Serial.begin(9600);

  pinMode(ledPinDoor1, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  connectToWiFi();
  client.setServer(mqtt_server, 1883);
  
  client.setCallback(callback);
  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  controlLED(ledPinDoor1, ledStateDoor1, ledOnTimeDoor1, 5000);

  //Triggering the relay
  if (relayOnTime > 0 && millis() - relayOnTime >= 3000) {
    digitalWrite(relayPin, LOW);
    Serial.println("Relé apagado");
    relayOnTime = 0;
  }
}

//Callback method to react messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  triggerRelay();  
}

//Method to connect to wifi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");
}

//Metehod to connect to mqtt
void connectToMQTT() {
  while (!client.connected()) {
    Serial.println("Conectando a MQTT...");
    if (client.connect("ArduinoClient", mqtt_username, mqtt_password)) {
      Serial.println("Conectado a MQTT");
      client.subscribe(mqtt_topic_door1);
    } else {
      Serial.print("Error al conectar: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}
//Method to control LED 
void controlLED(int pin, bool& state, unsigned long& onTime, unsigned long duration) {
  if (state && millis() - onTime >= duration) {
    state = false;
  }
  digitalWrite(pin, state);
}

//Method to trigger the relay
void triggerRelay(unsigned long duration) {
  digitalWrite(relayPin, HIGH);
  Serial.println("Relé activado");
  relayOnTime = millis();  
}
//Method to convert to json and send it to the client
void publishJson(const char* topic, StaticJsonDocument<200>& doc) {
  String jsonString;
  serializeJson(doc, jsonString);
  client.publish(topic, jsonString.c_str());
  Serial.println(jsonString);
}
//Making the doorbell's message 
void sendDoorbellMessage(const char* doorName) {
  StaticJsonDocument<200> doc;
  doc["type"] = "INFO";
  doc["src"] = doorName;
  doc["desc"] = "Doorbell ringing";
  doc["data"] = "";
  doc["time"] = millis() / 1000;
  doc["cmd"] = "event";
  doc["hostname"] = String(doorName) + "_topic";
  publishJson("door/events", doc);
}
//Sending the message to the publisher
void sendNotificationToPublisher(const char* message) {
  StaticJsonDocument<200> doc;
  doc["type"] = "INFO";
  doc["message"] = message;
  publishJson("door/notifications", doc);
}
