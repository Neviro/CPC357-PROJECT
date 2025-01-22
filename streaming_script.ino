#include <PubSubClient.h>
#include <WiFi.h>
#include "DHT.h"

#define DHTTYPE DHT11

const char* WIFI_SSID = "ssid"; // Your WiFi SSID
const char* WIFI_PASSWORD = "password"; // Your WiFi password
const char* MQTT_SERVER = "35.225.99.3"; // Your VM instance public IP address
const char* MQTT_TOPIC = "iot"; // MQTT topic for subscription
const int MQTT_PORT = 1883; // Non-TLS communication port
const int dht11Pin = 42; // DHT11 sensor pin

char buffer[128] = ""; // Text buffer
DHT dht(dht11Pin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200); // Initiate serial communication
    dht.begin(); // Initialize DHT sensor
    setup_wifi(); // Connect to the WiFi network
    client.setServer(MQTT_SERVER, MQTT_PORT); // Set up the MQTT client
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();
    delay(5000);

    float temperature = dht.readTemperature();
    sprintf(buffer, "Temperature: %.2f degree Celsius", temperature);
    client.publish(MQTT_TOPIC, buffer);
    Serial.println(buffer);
}

void reconnect() {
    while (!client.connected()) {
        Serial.println("Attempting MQTT connection...");

        if (client.connect("ESP32Client")) {
            Serial.println("Connected to MQTT server");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}
