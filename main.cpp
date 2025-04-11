#include <WiFi.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

// WiFi + MQTT
const char* ssid = "Nomad-Fi";
const char* password = "1234567890";
const char* mqtt_server = "192.168.137.114";

WiFiClient espClient;
PubSubClient client(espClient);

// Lamp pin
#define LAMP_PIN 2

// MQTT Topic
const char* lampTopic = "fastapi/topic";

// ===== DEBUG MACROS =====
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)

void startBLEAdvertising() {
  DEBUG_PRINTLN("Initializing BLE...");
  BLEDevice::init("ESP32_BLE_Advertiser");
  BLEServer *pServer = BLEDevice::createServer();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

  BLEAdvertisementData advertisementData;
  advertisementData.setName("ESP32_BLE_Advertiser");
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->start();
  DEBUG_PRINTLN("BLE advertising started.");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  DEBUG_PRINT("MQTT Message received [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("]: ");
  DEBUG_PRINTLN(message);

  if (String(topic) == lampTopic) {
    if (message == "on") {
      digitalWrite(LAMP_PIN, HIGH);
      DEBUG_PRINTLN("Lamp turned ON");
    } else if (message == "off") {
      digitalWrite(LAMP_PIN, LOW);
      DEBUG_PRINTLN("Lamp turned OFF");
    } else {
      DEBUG_PRINT("Unknown command: ");
      DEBUG_PRINTLN(message);
    }
  } else {
    DEBUG_PRINT("Unknown topic: ");
    DEBUG_PRINTLN(topic);
  }
}

void setup_wifi() {
  delay(10);
  DEBUG_PRINT("Connecting to WiFi: ");
  DEBUG_PRINTLN(ssid);
  WiFi.begin(ssid, password);

  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry_count++;
    if (retry_count > 20) {
      DEBUG_PRINTLN("WiFi connection failed. Restarting...");
      ESP.restart();  // restart if it takes too long
    }
  }
  DEBUG_PRINTLN("\nWiFi connected");
  DEBUG_PRINT("IP Address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    // Client ID must be unique
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      DEBUG_PRINTLN("connected");
      client.subscribe(lampTopic);
      DEBUG_PRINT("Subscribed to topic: ");
      DEBUG_PRINTLN(lampTopic);
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINT(client.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);  // More stable baud rate
  DEBUG_PRINTLN("Booting...");

  pinMode(LAMP_PIN, OUTPUT);
  digitalWrite(LAMP_PIN, LOW);
  DEBUG_PRINTLN("Lamp initialized to OFF.");

  startBLEAdvertising();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi disconnected! Reconnecting...");
    setup_wifi();  // Reconnect WiFi if dropped
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();  // Listen for MQTT
}
