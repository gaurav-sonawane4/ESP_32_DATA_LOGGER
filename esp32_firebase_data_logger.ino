/*

  Written by: gaurav sonawane 
  Created on: Nov 20, 2025
  ESP32 High Speed Data Logger (200Hz Sample Rate)
  For firebase setup refare this: https://drive.google.com/drive/folders/1U-ZA3UbW6PT3HV0Oiw-G-y4_pr3pjvhW?usp=sharing
  Github Resporetory: https://github.com/gaurav-sonawane4/ESP_32_DATA_LOGGER
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/RTDBHelper.h>
#include <addons/TokenHelper.h>
#include "driver/adc.h" 

// --- User Configuration ---
#define WIFI_SSID     "gaurav88"
#define WIFI_PASS     "gaurav88"
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL" 

// ADC Pin Definitions
#define CH_TEMP       ADC1_CHANNEL_0 
#define CH_LIGHT      ADC1_CHANNEL_1

// Timing Configuration
// Timer runs at 1MHz (1 tick = 1 microsecond)
#define TIMER_DIVIDER 1000000UL   
#define INTERVAL_US   5000UL      // 5000us = 5ms = 200Hz
#define BATCH_SEC     10          // Duration of one recording session
#define TOTAL_BATCHES 5           // Total sessions to record before stopping
#define PAUSE_MS      2000        // Delay between upload and next start

// Memory Management
// Calculate exactly how many rows we need to allocate.
// 10 seconds / 0.005s = 2000 samples.
const int MAX_SAMPLES = (BATCH_SEC * 1000) / (INTERVAL_US / 1000);

// Uses ~8 bytes per sample. 2000 samples = ~16KB RAM.
struct Row {
  uint32_t t;     // Timestamp (ms from start of batch)
  uint16_t val1;  // Sensor 1 Raw Value
  uint16_t val2;  // Sensor 2 Raw Value
};

// The main data buffer.
static Row buf[MAX_SAMPLES];

// ISR Control Variables
volatile int idx = 0;
volatile bool capturing = false;
volatile bool readyToSend = false;

hw_timer_t *timer = nullptr;

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// --- Interrupt Service Routine (ISR) ---
void IRAM_ATTR onTimer() {
  // If the main loop hasn't armed the system, do nothing.
  if (!capturing) return;

  // Safety check: if buffer is full, stop capturing immediately.
  if (idx >= MAX_SAMPLES) {
    capturing = false;
    readyToSend = true;
    return;
  }

  int r1 = adc1_get_raw(CH_TEMP);
  int r2 = adc1_get_raw(CH_LIGHT);

  buf[idx].t = idx * 5; 
  buf[idx].val1 = r1;
  buf[idx].val2 = r2;

  idx++;
  
  // Check if this was the last sample
  if (idx >= MAX_SAMPLES) {
    capturing = false;
    readyToSend = true;
  }
}


// Converts the binary buffer into a space-separated CSV string
// Format: "Time,Val1,Val2 Time,Val1,Val2 ..."
String makeCSV() {
  String s;
  // Reserve memory upfront to prevent heap fragmentation from 2000 reallocations
  s.reserve(MAX_SAMPLES * 15); 

  for (int i = 0; i < MAX_SAMPLES; i++) {
    s += String(buf[i].t) + "," + String(buf[i].val1) + "," + String(buf[i].val2);
    // Add space separator between rows, but not after the last one
    if (i < MAX_SAMPLES - 1) s += " "; 
  }
  return s;
}

// Uploads data to Firebase with simple retry logic
bool upload(String data, int batchNum) {
  String path = "/sensor_data/batch_" + String(batchNum);
  int retries = 3;

  while (retries > 0) {
    // Basic connection check
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
      delay(500); // Short wait for reconnection
    }

    Serial.print("Uploading batch " + String(batchNum) + "... ");
    
    // Attempt to write to Realtime Database
    if (Firebase.RTDB.setString(&fbdo, path.c_str(), data)) {
      Serial.println("Done.");
      return true;
    } else {
      Serial.print("Fail: ");
      Serial.println(fbdo.errorReason());
      retries--;
      // Wait a second before trying again to let network settle
      delay(1000);
    }
  }
  return false; // All retries failed
}

// --- Setup Phase ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarting Data Logger...");

  // ADC Configuration
  analogReadResolution(12);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(CH_TEMP, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(CH_LIGHT, ADC_ATTEN_DB_11);

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" OK");

  // Firebase Initialization
  config.api_key = API_KEY;
  config.database_url = DB_URL;
  
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  timer = timerBegin(TIMER_DIVIDER);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, INTERVAL_US, true, 0);
  timerStop(timer); 

  Serial.println("Setup complete. Waiting for main loop...");
}

// --- Main Loop ---
void loop() {
  static int batchCount = 0;

  // Exit condition: Stop everything after total batches reached
  if (batchCount >= TOTAL_BATCHES) {
    timerStop(timer);
    Serial.println("All batches finished.");
    while(1) delay(1000); // Infinite loop to halt execution
  }

  //Start Recording
  // Condition: We are not capturing and we don't have data waiting to send
  if (!capturing && !readyToSend) {
    Serial.println("\n--- Recording Batch " + String(batchCount + 1) + " ---");
    
    // Reset counters
    idx = 0;
    readyToSend = false;
    capturing = true;
    
    // Enable the hardware timer to start ISR triggers
    timerRestart(timer);
    timerStart(timer);
  }

  // Process Data
  // Condition: ISR has set the 'readyToSend' flag
  if (readyToSend) {
    timerStop(timer); // Stop timer so ISR doesn't corrupt buffer while reading
    capturing = false;

    Serial.println("Buffer full. Building CSV...");
    
    // Convert raw data to CSV
    String csv = makeCSV();
    Serial.print("Size: "); Serial.println(csv.length());

    // Perform blocking upload
    if (!upload(csv, batchCount + 1)) {
      Serial.println("Upload failed gave up.");
    }

    batchCount++;
    readyToSend = false; // Reset flag to allow next batch

    //Cooldown
    if (batchCount < TOTAL_BATCHES) {
      Serial.println("Cooling down...");
      delay(PAUSE_MS);
    }
  }
  delay(10);
}
