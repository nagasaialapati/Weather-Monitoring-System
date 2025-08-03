#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "SparkFunBME280.h"

BME280 mySensor;

const int uvSensorPin = 34; // Change this to the appropriate GPIO pin on ESP32
const int gasSensorPin = 32; // Change this to the appropriate GPIO pin on ESP32

// Replace with your network credentials
const char* ssid = "IIT_Bhilai";
const char* password = "iitbhilai2023";

// Replace with your Google Apps Script web app URL
const char* serverName = "https://script.google.com/macros/s/AKfycbzY3Jcaa6XmK9UGCyrQGIsHJgrWnhzRnahHjrmKgdC5rnkL79bOd4l6_q7qSPa7-8OTHg/exec";

// Function to map sensor resistance to gas concentrations (in ppm)
float resistanceToGasConcentration(float Rs, String gas) {
  if (gas == "CO") {
    return pow(10, ((log10(Rs) - 0.4) / (-0.8)));
  } else if (gas == "H2") {
    return pow(10, ((log10(Rs) - 0.2) / (-0.6)));
  } else if (gas == "CH4") {
    return pow(10, ((log10(Rs) - 0.3) / (-0.5)));
  } else if (gas == "C2H6OH") {
    return pow(10, ((log10(Rs) - 0.5) / (-0.7)));
  }
  return 0;
}

// Define a function to map voltage to UV index
int voltageToUVIndex(float voltage) {
  if (voltage < 50) return 0;
  else if (voltage < 227) return 1;
  else if (voltage < 318) return 2;
  else if (voltage < 408) return 3;
  else if (voltage < 503) return 4;
  else if (voltage < 606) return 5;
  else if (voltage < 696) return 6;
  else if (voltage < 795) return 7;
  else if (voltage < 881) return 8;
  else if (voltage < 976) return 9;
  else if (voltage < 1079) return 10;
  else return 11; // 1170+ mV corresponds to UV Index 11+
}

void setup() {
  Serial.begin(115200);
  Serial.println("Reading basic values from BME280");

  Wire.begin();

  if (mySensor.beginI2C() == false) { // Begin communication over I2C
    Serial.println("The sensor did not respond. Please check wiring.");
    while(1); // Freeze
  }
  
  analogReadResolution(12); // Set ADC resolution to 12 bits

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  // Read BME280 sensor data
  float humidity = mySensor.readFloatHumidity();
  float pressure = mySensor.readFloatPressure();
  float altitude = mySensor.readFloatAltitudeFeet();
  float temperature = mySensor.readTempC();

  // Read the analog value from the UV sensor
  int uvSensorValue = analogRead(uvSensorPin);
  float uvVoltage = uvSensorValue * (3300.0 / 4095.0); // 3.3V reference and 12-bit resolution
  int uvIndex = voltageToUVIndex(uvVoltage);
  
  // Read the analog value from the MiCS-5524 sensor
  int gasSensorValue = analogRead(gasSensorPin);
  float Rs = gasSensorValue * (10000.0 / 4095.0); // Assuming a load resistor of 10k ohms and 12-bit ADC

  // Calculate the gas concentrations
  float CO_concentration = resistanceToGasConcentration(Rs, "CO");
  float H2_concentration = resistanceToGasConcentration(Rs, "H2");
  float CH4_concentration = resistanceToGasConcentration(Rs, "CH4");
  float C2H6OH_concentration = resistanceToGasConcentration(Rs, "C2H6OH");

  // Print sensor data to Serial
  Serial.print("Humidity: ");
  Serial.print(humidity, 0);
  Serial.print(" %");

  Serial.print(" Pressure: ");
  Serial.print(pressure, 0);
  Serial.print(" Pa");

  Serial.print(" Altitude: ");
  Serial.print(altitude, 1);
  Serial.print(" ft");

  Serial.print(" Temp: ");
  Serial.print(temperature, 2);
  Serial.println(" C");

  Serial.print("UV Sensor Voltage: ");
  Serial.print(uvVoltage);
  Serial.println(" mV");

  Serial.print("UV Index: ");
  Serial.println(uvIndex);

  Serial.print("CO Concentration: ");
  Serial.print(CO_concentration);
  Serial.println(" ppm");

  Serial.print("H2 Concentration: ");
  Serial.print(H2_concentration);
  Serial.println(" ppm");

  Serial.print("CH4 Concentration: ");
  Serial.print(CH4_concentration);
  Serial.println(" ppm");

  Serial.print("Ethanol Concentration: ");
  Serial.print(C2H6OH_concentration);
  Serial.println(" ppm");

  // Prepare JSON payload
  String jsonPayload = "{\"temperature\":";
  jsonPayload += String(temperature);
  jsonPayload += ",\"humidity\":";
  jsonPayload += String(humidity);
  jsonPayload += ",\"pressure\":";
  jsonPayload += String(pressure);
  jsonPayload += ",\"altitude\":";
  jsonPayload += String(altitude);
  jsonPayload += ",\"uvIndex\":";
  jsonPayload += String(uvIndex);
  jsonPayload += ",\"CO\":";
  jsonPayload += String(CO_concentration);
  jsonPayload += ",\"H2\":";
  jsonPayload += String(H2_concentration);
  jsonPayload += ",\"CH4\":";
  jsonPayload += String(CH4_concentration);
  jsonPayload += ",\"ethanol\":";
  jsonPayload += String(C2H6OH_concentration);
  jsonPayload += "}";

  // Send HTTP POST request
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  // Wait 10 seconds before taking the next reading
  delay(10000);
  Serial.print("\n");
}