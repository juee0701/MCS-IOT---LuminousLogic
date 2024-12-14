#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Juee"  
#define AIO_KEY         "key"  

// Pin definitions
#define LIGHT_SENSOR_PIN 36  
#define LED_PIN 21           
#define TOUCH_PIN T0         

// WiFi credentials
const char* ssid = "Juee";  
const char* password = "Juee2025";  

// Server details
const char* serverUrl = "https://jueea.pythonanywhere.com/update_sensor";  

// Variables
int lightValue = 0;          // Variable to store light sensor reading
int ledBrightness = 0;       // Calculated LED brightness (0-255)
String currentMode = "adaptive"; // Modes: adaptive, dim, read, full
int touchThreshold = 40;    // Adjust based on touch sensor sensitivity
unsigned long lastTouchTime = 0;
const unsigned long debounceDelay = 500; // Debounce delay in milliseconds

// MQTT client
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe LEDMode = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/LED_Mode");

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);

    // Configure pin modes
    pinMode(LIGHT_SENSOR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    touchAttachInterrupt(TOUCH_PIN, handleTouch, touchThreshold);

    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    mqtt.subscribe(&LEDMode);
    Serial.println("Adaptive Light System Initialized");
}

void handleTouch() {
    unsigned long currentTime = millis();
    if (currentTime - lastTouchTime > debounceDelay) {
        // Cycle through modes
        if (currentMode == "adaptive") {
            currentMode = "dim";
        } else if (currentMode == "dim") {
            currentMode = "read";
        } else if (currentMode == "read") {
            currentMode = "full";
        } else {
            currentMode = "adaptive";
        }
        Serial.print("Mode switched to: ");
        Serial.println(currentMode);
        lastTouchTime = currentTime;
    }
}

void sendToServer(int sensorValue) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{\"light_value\": " + String(sensorValue) + ", \"led_brightness\": " + String(ledBrightness) + ", \"mode\": \"" + currentMode + "\"}";

        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            Serial.print("Server response: ");
            Serial.println(httpResponseCode);
            Serial.println(http.getString());
        } else {
            Serial.print("Error sending to server: ");
            Serial.println(http.errorToString(httpResponseCode).c_str());
        }

        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}

void loop() {
    MQTT_connect();

    // Check for incoming MQTT messages
    Adafruit_MQTT_Subscribe* subscription;
    while ((subscription = mqtt.readSubscription(5000))) {
        if (subscription == &LEDMode) {
            String command = (char*)LEDMode.lastread;
            Serial.print("Received command: ");
            Serial.println(command);

            // Process voice command
            if (command == "adaptive") {
                currentMode = "adaptive";
            } else if (command == "dim") {
                currentMode = "dim";
            } else if (command == "read") {
                currentMode = "read";
            } else if (command == "full") {
                currentMode = "full";
            } else if (command == "off") {
                ledBrightness = 0;
                currentMode = "off";
            }
        }
    }

    // Read the ambient light level
    lightValue = analogRead(LIGHT_SENSOR_PIN);  

    // Adjust LED brightness based on mode
    if (currentMode == "adaptive") {
        ledBrightness = map(lightValue, 0, 4095, 255, 0); // Invert brightness (darker = brighter LED)
    } else if (currentMode == "dim") {
        ledBrightness = 50; // Low brightness
    } else if (currentMode == "read") {
        ledBrightness = 150; // Medium brightness for reading
    } else if (currentMode == "full") {
        ledBrightness = 255; // Full brightness
    }

    // Write the brightness to the LED using PWM
    analogWrite(LED_PIN, ledBrightness);

    // Send the sensor data to the server
    sendToServer(lightValue);

    // Debug output
    Serial.print("Light Value: ");
    Serial.print(lightValue);
    Serial.print(" | LED Brightness: ");
    Serial.print(ledBrightness);
    Serial.print(" | Current Mode: ");
    Serial.println(currentMode);

    delay(1000);  // Delay to send data at 1-second intervals
}

void MQTT_connect() {
    while (mqtt.connected() == false) {
        Serial.print("Connecting to MQTT...");
        if (mqtt.connect()) {
            Serial.println("Connected!");
        } else {
            Serial.print("Failed, rc=");
            Serial.println(" Trying again in 5 seconds.");
            delay(5000);
        }
    }
}
