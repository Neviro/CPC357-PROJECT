#include <lcd_i2c.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

// LCD address
lcd_i2c lcd(0x3E, 16, 1);

// Control barrier
Servo barrierServo;
const int servoPin = 4;
const int servoOpenAngle = 90;
const int servoCloseAngle = 180;

// To open barrier (IR sensor 1)
const int irBarrier1 = 5;

// To close barrier (IR sensor 2)
const int irBarrier2 = 6;

// To detect available parking slot on floor 1 (IR sensor 3)
const int irParking1 = 7;

// To detect available parking slot on floor 2 (IR sensor 4)
const int irParking2 = 8;

// To detect air quality of vehicle
const int airQualityPin = 9;

// Counter for number of cars
int total_car_count = 0;

// Counter for number of cars
int ev_car_count = 0;

// Counter for number of cars
int petrol_car_count = 0;

// WiFi and MQTT configuration
const char* WIFI_SSID = ""; // Your your WIFI SSID/Name here
const char* WIFI_PASSWORD = ""; // Your WIFI Password here
const char* MQTT_SERVER = ""; // Your MQTT server IP address here
const int MQTT_PORT = 1883; // Your MQTT server port here
const char* MQTT_TOPIC = "iot"; // Your MQTT topic here

WiFiClient espClient;
PubSubClient client(espClient);

void scrollText(String message, int scrollDelay, int repeatCount = 2);

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
    lcd.begin();
    lcd.clear();
    Serial.begin(115200);

    pinMode(irBarrier1, INPUT);
    pinMode(irBarrier2, INPUT);
    pinMode(irParking1, INPUT);
    pinMode(irParking2, INPUT);
    pinMode(airQualityPin, INPUT);

    barrierServo.attach(servoPin);
    barrierServo.write(servoCloseAngle);

    lcd.print("System Ready");
    Serial.println("System Ready");
    delay(2000);
    lcd.clear();

    setup_wifi();
    client.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }

    client.loop();

    // Read sensor values
    int barrier1State = digitalRead(irBarrier1);
    int barrier2State = digitalRead(irBarrier2);
    int airQualityValue = analogRead(airQualityPin);
    bool parking1Full = digitalRead(irParking1);
    bool parking2Full = digitalRead(irParking2);

    // Serial monitor output
    Serial.print("Barrier1: ");
    Serial.print(barrier1State);
    Serial.print(" Barrier2: ");
    Serial.print(barrier2State);
    Serial.print(" AirQuality: ");
    Serial.print(airQualityValue);
    Serial.print(" P1 Full: ");
    Serial.print(parking1Full);
    Serial.print(" P2 Full: ");
    Serial.println(parking2Full);


    // Detect car at entry (IR 1 = 0 when car is present)
    if (barrier1State == 0) {
        total_car_count++;
        lcd.clear();
        lcd.print("Car Detected!");
        Serial.println("Car Detected at Entry");
        barrierServo.write(servoOpenAngle);
        Serial.println("Barrier Opened");
        delay(2000);

        // Good air quality (EV)
        if (airQualityValue < 1300) {
            // Floor 1 available
            if (parking1Full == 1) {
                ev_car_count++;
                lcd.clear();
                scrollText("EV->1st Floor", 300, 2);
                Serial.println("EV (Good Air Quality) -> Floor 1");
            } else {
                ev_car_count++;
                lcd.clear();
                scrollText("1st Full->Redirect to Floor 2", 300, 2);
                Serial.println("Floor 1 Full -> Redirect to Floor 2");
            }
        } else {
            // Poor air quality (non-EV)
            if (parking2Full == 1) {
                petrol_car_count++;
                lcd.clear();
                scrollText("Non-EV->2nd Floor", 300, 2);
                Serial.println("Non-EV (Poor Air Quality) -> Floor 2");
            } else {
                petrol_car_count++;
                lcd.clear();
                scrollText("2nd Full->Redirect to Floor 1", 300, 2);
                Serial.println("Floor 2 Full -> Redirect to Floor 1");
            }
        }
        /// MQTT message in JSON format
        char mqttMessage[256];
        snprintf(mqttMessage, sizeof(mqttMessage), 
            "{\"TotalCar\": %d, \"EvCar\": %d, \"PetrolCar\": %d, \"AirQuality\": %d, \"Parking1Full\": %d, \"Parking2Full\": %d}", 
            total_car_count, ev_car_count, petrol_car_count, airQualityValue, parking1Full, parking2Full);
        Serial.println(mqttMessage);
        client.publish(MQTT_TOPIC, mqttMessage);

        // Wait for the car to pass IR2
        while (digitalRead(irBarrier2) == 1) {
            scrollText("Waiting car to exit", 300, 2);
            Serial.println("Waiting for car to exit...");
        }

        barrierServo.write(servoCloseAngle);
        lcd.clear();
        scrollText("Barrier Closed", 300, 2);
        Serial.println("Barrier Closed");
        delay(2000);
        delay(20000);
    }
}

void scrollText(String message, int scrollDelay, int repeatCount) {
    int messageLength = message.length();
    if (messageLength <= 16) {
        for (int j = 0; j < repeatCount; j++) {
            lcd.clear();
            lcd.print(message.c_str());
            delay(scrollDelay * 5);
        }
        return;
    }
    for (int r = 0; r < repeatCount; r++) {
        for (int i = 0; i <= messageLength - 16; i++) {
            lcd.clear();
            lcd.print(message.substring(i, i + 16).c_str());
            delay(scrollDelay);
        }
    }
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
