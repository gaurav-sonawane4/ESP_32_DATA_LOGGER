# ESP_32_DATA_LOGGER
ESP32 Data Logger (200Hz)

A simple logger that reads two sensors every 5ms (200Hz) using hardware timers. It saves 10 seconds of data to RAM and then uploads it to Firebase as a CSV string.

How it works

Samples 2 analog channels using adc1_get_raw (fast & interrupt-safe).

Buffers 2000 data points in memory.

Uploads the batch to Firebase at /sensor_data/batch_X.

Repeats for 5 batches.

Hardware Setup

ESP32 Board

Sensors: Must use ADC1 pins (GPIO 32-39).

Default: GPIO 36 and 37.

Do not use ADC2 pins (like GPIO 4, 2, 15) or they will fail when WiFi is on.

Setup Instructions

Install Library:
Search for Firebase Arduino Client Library for ESP8266 and ESP32 in the library manager.

Config:
Edit Humanized_Data_Logger.ino with your WiFi and Firebase details.

Need Firebase Help?
Check this setup guide

Run:
Flash the code. Open Serial Monitor (115200 baud) to see progress.

Output Format

The data is saved as a space-separated string to save DB write ops:
0,1200,300 5,1205,310 10,1210,320 ...
(Format: TimeMS, Sensor1, Sensor2)

Notes

The code uses IRAM_ATTR for the timer interrupt to keep it stable.

Don't add Serial.print inside the onTimer function or it will crash.
