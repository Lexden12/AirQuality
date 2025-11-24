# AirQuality

The goal of this project is to build an air quality meter for use in the home.

The details of this project can be found on my [website](https://lexden.dev/projects/airquality).

This repo will focus exclusively on the firmware for this project and how to use it.

### Usage

To use this sketch in Arduino IDE, you must add the dependencies to your libraries:
- Adafruit_SSD1325_Library
- Adafruit_LPS2X
- ESPDateTime
- arduino-i2c-sgp41
- arduino-i2c-scd4x

Then, simply edit the ssid and password for your WiFi on lines 27 and 28. Upload the code to your ESP32, and it should connect to WiFi and then start displaying data.

### Roadmap

Currently, this is a minimal text-based output on an OLED panel, with heavy reliance on several third-party libraries.
This sketch has not been tested beyond compilation. A sketch with only the display showing date/time is what I have used thus far. I will test the full sketch and update according to the following roadmap once I receive the sensors.
Primary improvement:
- Develop a web server to receive data posted to it by the sensor(s) to allow for historical data monitoring
Secondary improvements:
- Turn this sketch into a proper Arduino library using generalized libraries
- Reduce reliance on so many third-party libraries which add a lot of bloat (currently uses 76% of ESP32C3 program memory)
- Find a good way to improve the UI (icons rather than text, multiple UI options)

Copyright (c) 2025 Alex "Lexden" Schendel
