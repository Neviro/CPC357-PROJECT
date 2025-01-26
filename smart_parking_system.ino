#include <lcd_i2c.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <mbedtls/aes.h>

// LCD address
lcd_i2c lcd(0x3E, 16, 1);

// Control barrier
Servo barrierServo;
const int servoPin = 4;
const int servoOpenAngle = 90;
const int servoCloseAngle = 180;

// Sensor pins
const int irBarrier1 = 5;
const int irBarrier2 = 6;
const int irParking1 = 7;
const int irParking2 = 8;
const int airQualityPin = 9;

// Counters
int total_car_count = 0;
int ev_car_count = 0;
int petrol_car_count = 0;

// WiFi and MQTT configuration
const char* WIFI_SSID = "your_wifi_ssid";
const char* WIFI_PASSWORD = "your_wifi_password";
const char* MQTT_SERVER = "your_mqtt_server";
const int MQTT_PORT = 8883;
const char* MQTT_TOPIC = "iot_secure";
const char* MQTT_USERNAME = "your_mqtt_username";
const char* MQTT_PASSWORD = "your_mqtt_password";

// Encryption key (AES-128)
const unsigned char AES_KEY[16] = "your_16_byte_key";

// WiFi and MQTT clients
WiFiClientSecure espClient;
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

    Serial.println();
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void encryptData(const char* input, unsigned char* output, size_t length) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, AES_KEY, 128);

    unsigned char iv[16] = {0}; // Initialization vector
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, length, iv, (unsigned char*)input, output);

    mbedtls_aes_free(&aes);
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
    espClient.setCACert("your_ca_certificate_here"); // Add CA certificate
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
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

    if (barrier1State == 0) {
        total_car_count++;
        lcd.clear();
        lcd.print("Car Detected!");
        Serial.println("Car Detected at Entry");
        barrierServo.write(servoOpenAngle);
        delay(2000);

        if (airQualityValue < 1300) {
            if (parking1Full) {
                ev_car_count++;
                lcd.clear();
                scrollText("EV->1st Floor", 300, 2);
            } else {
                ev_car_count++;
                lcd.clear();
                scrollText("1st Full->Redirect to Floor 2", 300, 2);
            }
        } else {
            if (parking2Full) {
                petrol_car_count++;
                lcd.clear();
                scrollText("Non-EV->2nd Floor", 300, 2);
            } else {
                petrol_car_count++;
                lcd.clear();
                scrollText("2nd Full->Redirect to Floor 1", 300, 2);
            }
        }

        // Create MQTT message
        char mqttMessage[256];
        snprintf(mqttMessage, sizeof(mqttMessage),
                 "{\"TotalCar\": %d, \"EvCar\": %d, \"PetrolCar\": %d, \"AirQuality\": %d, \"Parking1Full\": %d, \"Parking2Full\": %d}",
                 total_car_count, ev_car_count, petrol_car_count, airQualityValue, parking1Full, parking2Full);

        // Encrypt the message
        unsigned char encryptedMessage[256];
        encryptData(mqttMessage, encryptedMessage, sizeof(mqttMessage));

        // Publish encrypted message
        client.publish(MQTT_TOPIC, (char*)encryptedMessage);

        while (digitalRead(irBarrier2) == 1) {
            scrollText("Waiting car to exit", 300, 2);
        }

        barrierServo.write(servoCloseAngle);
        lcd.clear();
        scrollText("Barrier Closed", 300, 2);
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
        if (client.connect("ESP32Client", MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("Connected to MQTT server");
            client.subscribe(MQTT_TOPIC);
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            delay(5000);
        }
    }
}
