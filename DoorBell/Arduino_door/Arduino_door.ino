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

// Function prototypes
void connectToWiFi();
void connectToMQTT();
void controlLED(int pin, bool& state, unsigned long& onTime, unsigned long duration);
void triggerRelay(unsigned long duration = 3000);
void callback(char* topic, byte* payload, unsigned int length);
void publishJson(const char* topic, StaticJsonDocument<200>& doc);
void sendDoorbellMessage(const char* doorName);
void sendNotificationToPublisher(const char* message);

// Setup
void setup() {
  pinMode(ledPinDoor1, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  connectToWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  connectToMQTT();
}

// Main loop
void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  controlLED(ledPinDoor1, ledStateDoor1, ledOnTimeDoor1, 5000);

  // Turn off relay after 3 seconds
  if (relayOnTime > 0 && millis() - relayOnTime >= 3000) {
    digitalWrite(relayPin, LOW);
    relayOnTime = 0;
  }
}

// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, message)) return;

  const char* desc = doc["desc"] | "";
  const char* cmd  = doc["cmd"]  | "";

  if (strcmp(desc, "Doorbell ringing") == 0 && strcmp(cmd, "event") == 0) {
    triggerRelay();
  }
}

// Wi-Fi connection
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// MQTT connection
void connectToMQTT() {
  while (!client.connected()) {
    if (client.connect("ArduinoClient", mqtt_username, mqtt_password)) {
      client.subscribe(mqtt_topic_door1);
    } else {
      delay(2000);
    }
  }
}

// LED control
void controlLED(int pin, bool& state, unsigned long& onTime, unsigned long duration) {
  if (state && millis() - onTime >= duration) {
    state = false;
  }
  digitalWrite(pin, state);
}

// Relay trigger
void triggerRelay(unsigned long duration) {
  digitalWrite(relayPin, HIGH);
  relayOnTime = millis();
}

// Publish JSON
void publishJson(const char* topic, StaticJsonDocument<200>& doc) {
  String jsonString;
  serializeJson(doc, jsonString);
  client.publish(topic, jsonString.c_str());
}

// Send doorbell event
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

// Send notification
void sendNotificationToPublisher(const char* message) {
  StaticJsonDocument<200> doc;
  doc["type"] = "INFO";
  doc["message"] = message;
  publishJson("door/notifications", doc);
}
