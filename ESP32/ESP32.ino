#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// Pin Definitions
#define DHTPIN 4          // DHT11 data pin
#define DHTTYPE DHT11     // DHT11 sensor type
#define GAS_SENSOR_PIN 35 // MQ-6 gas sensor analog pin
#define VOLTAGE_PIN 34    // Voltage sensor analog pin
#define SOIL_MOISTURE_1_PIN 32 // Soil moisture sensor 1 analog pin
#define SOIL_MOISTURE_2_PIN 33 // Soil moisture sensor 2 analog pin
#define RELAY_MOTOR_PIN 25     // Relay 1 (Motor) control pin
#define RELAY_WATER_PUMP_PIN 26 // Relay 2 (Water Pump) control pin
#define LED_PIN 14             // LED pin
#define ANALOG_IN_PIN  27 // ESP32 pin GPIO36 (ADC0) connected to voltage sensor
#define REF_VOLTAGE    3.3
#define ADC_RESOLUTION 4096.0
#define R11             30000.0 // resistor values in voltage sensor (in ohms)
#define R12             7500.0  // resistor values in voltage sensor (in ohms)

// Constants
#define MAX_ADC 4095       // Max ADC value for ESP32 (12-bit)
#define REF_VOLTAGE 3.3    // ESP32 ADC reference voltage
#define TEMP_THRESHOLD_HIGH 30 // Temperature threshold to turn on motor
#define GAS_THRESHOLD   1000    // Gas threshold to turn on LED
#define SOIL_MOISTURE_THRESHOLD 45 // Soil moisture threshold to turn on water pump (in percentage)


// WiFi credentials
const char* ssid = "ACD";
const char* password = "pass1234";

// Server details
const char* serverUrl = "http://192.168.43.251:5000/data";

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    // Initialize serial communication
    Serial.begin(115200);

    // Initialize DHT sensor
    dht.begin();

    // Set pin modes
    pinMode(GAS_SENSOR_PIN, INPUT);
    pinMode(VOLTAGE_PIN, INPUT);
    pinMode(SOIL_MOISTURE_1_PIN, INPUT);
    pinMode(SOIL_MOISTURE_2_PIN, INPUT);
    pinMode(RELAY_MOTOR_PIN, OUTPUT);
    pinMode(RELAY_WATER_PUMP_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    analogSetAttenuation(ADC_11db);

    // Ensure relays are off initially
    digitalWrite(RELAY_MOTOR_PIN, LOW);
    digitalWrite(RELAY_WATER_PUMP_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    // Connect to Wi-F
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
}

void loop() {
    // Read DHT11 sensor data
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Read gas sensor value
    int gasValue = analogRead(GAS_SENSOR_PIN);

    // Read voltage sensor value
    int adcValue = analogRead(VOLTAGE_PIN);
    float voltage = (adcValue / (float)MAX_ADC) * REF_VOLTAGE * 2; // Multiply by 2 to get actual battery voltage
    int batteryLevel = map(voltage * 100, 200, 370, 0, 100); // Convert voltage to battery percentage
    batteryLevel = constrain(batteryLevel, 0, 100);

    // Read soil moisture sensor values
    int soilMoisture1 = analogRead(SOIL_MOISTURE_1_PIN);
    int soilMoisture2 = analogRead(SOIL_MOISTURE_2_PIN);
    int moisture1 = (100 - ((soilMoisture1 / 4095.00) * 100)); // Convert to percentage
    int moisture2 = (100 - ((soilMoisture2 / 4095.00) * 100)); // Convert to percentage

    // Control motor relay based on temperature
    if (temperature > TEMP_THRESHOLD_HIGH) {
        digitalWrite(RELAY_MOTOR_PIN, HIGH); // Turn on motor
    } else {
        digitalWrite(RELAY_MOTOR_PIN, LOW); // Turn off motor
    }

    // Control LED based on gas sensor value
    if (gasValue >= 3000)  {
        digitalWrite(LED_PIN, HIGH); // Turn on LED
        Serial.println("The gas is present");
    } else {
        digitalWrite(LED_PIN, LOW); // Turn off LED
        Serial.println("The gas is NOT present");
    }

    // Control water pump relay based on soil moisture
    if (moisture1 < SOIL_MOISTURE_THRESHOLD || moisture2 < SOIL_MOISTURE_THRESHOLD) {
        digitalWrite(RELAY_WATER_PUMP_PIN, HIGH); // Turn on water pump
    } else {
        digitalWrite(RELAY_WATER_PUMP_PIN, LOW); // Turn off water pump
    }

    // read the analog input
    int adc_value = analogRead(ANALOG_IN_PIN);

    // determine voltage at adc input
    float voltage_adcc = ((float)adc_value * REF_VOLTAGE) / ADC_RESOLUTION;

    // calculate voltage at the sensor input
    float voltage_inn = voltage_adcc * (R11 + R12) / R12;

    // Print all sensor values to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C | Humidity: ");
    Serial.print(humidity);
    Serial.print(" % | Gas Value: ");
    Serial.print(gasValue);
    Serial.print("  | Battery Voltage: ");
    Serial.print(voltage);
    Serial.print(" V | Battery Level: ");
    Serial.print(batteryLevel);
    Serial.print(" % | Soil Moisture 1: ");
    Serial.print(moisture1);
    Serial.print(" % | Soil Moisture 2: ");
    Serial.print(moisture2);
    Serial.println("%");

    // Send data to server
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        String jsonData = "{\"temperature\":" + String(temperature) + 
                          ",\"humidity\":" + String(humidity) + 
                          ",\"gasValue\":" + String(gasValue) + 
                          ",\"voltage\":" + String(voltage) + 
                          ",\"batteryLevel\":" + String(batteryLevel) + 
                          ",\"moisture1\":" + String(moisture1) + 
                          ",\"moisture2\":" + String(moisture2) + 
                          ",\"relayMotor\":" + String(digitalRead(RELAY_MOTOR_PIN)) + 
                          ",\"relayWaterPump\":" + String(digitalRead(RELAY_WATER_PUMP_PIN)) + 
                          ",\"solarVoltage\":" + String(voltage_inn) + "}";

        int httpResponseCode = http.POST(jsonData);
        if (httpResponseCode > 0) {
            Serial.println("Data sent to server");
        } else {
            Serial.println("Error sending data to server");
        }
        http.end();
    }

    // Delay for 2 seconds
    delay(2000);
}