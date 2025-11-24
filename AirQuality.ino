/*
Copyright 2025 Alex "Lexden" Schendel

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_SSD1325.h>
#include "ESPDateTime.h"
#include <SensirionI2cScd4x.h>
#include <SensirionI2CSgp41.h>
#include <Wire.h>
#include <Adafruit_LPS2X.h>
#include <Adafruit_Sensor.h>

char ssid[] = "ssid";     //  your network SSID (name)
char pass[] = "password";  // your network password

// These are needed for both hardware & softare SPI
#define OLED_CS D3
#define OLED_RESET D4
#define OLED_DC D0

Adafruit_SSD1325 display(OLED_DC, OLED_RESET, OLED_CS);

SensirionI2CSgp41 sgp41;
// time in seconds needed for NOx conditioning (9 seconds)
uint16_t conditioning_s = 9;
char errorMessageSGP[256];
uint16_t srawVoc = 0;
uint16_t srawNox = 0;

SensirionI2cScd4x sensor; // CO2+Temp+RH Sensor
uint16_t co2Concentration = 0;
float temperature = 0.0;
float relativeHumidity = -1.0;
static char errorMessage[64];
static uint16_t error;

Adafruit_LPS22 lps; // Barometric Pressure Sensor
sensors_event_t temp;
sensors_event_t pressure;

int loopCount = 0;

void setup() {                
  Serial.begin(9600);

  display.begin();

  display.command(SSD1325_SETCONTRAST); // set contrast current
  display.command(0);

  display.setRotation(1);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Connecting to WiFi");
  delay(1000);
  WiFi.begin(ssid, pass);
  
  Wire.begin();
  sensor.begin(Wire, SCD41_I2C_ADDR_62);
  error = sensor.wakeUp();
  if (error) {
    Serial.print("Error trying to execute wakeUp(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }

  sgp41.begin(Wire);
  uint16_t testResult;
  error = sgp41.executeSelfTest(testResult);
  if (error) {
    Serial.print("Error trying to execute executeSelfTest(): ");
    errorToString(error, errorMessageSGP, 256);
    Serial.println(errorMessageSGP);
  } else if (testResult != 0xD400) {
    Serial.print("executeSelfTest failed with error: ");
    Serial.println(testResult);
  }

  if (!lps.begin_I2C()) {
    Serial.println("Failed to find LPS22 chip");
  }
  
  while (WiFi.status() != WL_CONNECTED){
    display.print(".");
    display.display();
    delay(1000);
  }
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connected!");
  display.display();
  delay(1000);
  DateTime.setServer("us.pool.ntp.org");
  DateTime.setTimeZone("PST8PDT,M3.2.0,M11.1.0");
  DateTime.begin();

  lps.setDataRate(LPS22_RATE_1_HZ);

  error = sensor.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
}


void loop() {
  // get pressure data every ten cycles (once per second)
  if(loopCount % 10 == 0){
    lps.getEvent(&pressure, &temp);// get pressure
  }
  // get CO2 Concentration+Temp+RH every 100 cycles (once per ten seconds)
  if(loopCount % 100 == 0){
    bool dataReady = false;
    error = sensor.getDataReadyStatus(dataReady);
    if (!error && dataReady) {
      error = sensor.setAmbientPressure(((uint16_t)pressure.pressure) * 100); // set ambient pressure of CO2 sensor in Pa based on hPa reading from LPS22
      error = sensor.readMeasurement(co2Concentration, temperature, relativeHumidity);
      if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
      }
    }
  }
  // get NOx and VOC data every ten cycles (once per second), but conditioning requires RH, so wait to make sure we have valid RH data
  if(loopCount % 10 == 0 && relativeHumidity >= 0.0){
    if (conditioning_s > 0) {
        // During NOx conditioning (10s) SRAW NOx will remain 0
        error = sgp41.executeConditioning((uint16_t)(relativeHumidity * 65535 / 100), (uint16_t)((temperature+45) * 65535 / 175), srawVoc);
        conditioning_s--;
    } else {
        // Read Measurement
        error = sgp41.measureRawSignals((uint16_t)(relativeHumidity * 65535 / 100), (uint16_t)((temperature+45) * 65535 / 175), srawVoc, srawNox);
    }
    if (error) {
      Serial.print("Error trying to execute measureRawSignals(): ");
      errorToString(error, errorMessageSGP, 256);
      Serial.println(errorMessageSGP);
    }
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(DateTime.format("%A"));
  display.println(DateTime.format("%x"));
  display.println(DateTime.format("%X"));
  display.printf("LPS: %.1f°C\n", temp.temperature);
  display.printf("SCD: %.1f°C\n", temperature);
  display.printf("%.1fhPa\n", pressure.pressure);
  display.printf("CO2: %uppm\n", co2Concentration);
  display.printf("%.1f%%RH\n", relativeHumidity);
  if(conditioning_s > 0){
    display.println("NOx: Calibrating");
    display.println("VOC: Calibrating");
  }
  else{
    display.printf("NOx: %u\n", srawNox);
    display.printf("VOC: %u\n", srawVoc);
  }
  display.display();
  loopCount++;
  loopCount = loopCount % 100;
  delay(100);
}