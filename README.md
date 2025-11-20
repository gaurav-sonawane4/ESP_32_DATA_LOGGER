# ESP32 Data Logger (200Hz)

A high-speed data logger that samples **two analog sensors every 5ms (200Hz)** using an ESP32 hardware timer.  
It stores **10 seconds of data in RAM** and then uploads it to **Firebase Realtime Database** as a **compressed CSV string**.

---

## 🚀 Features
- Samples **2 analog channels** using `adc1_get_raw` (fast & interrupt-safe)
- Buffers **2000 samples** in memory per batch
- Uploads batches to Firebase at:  
  `/sensor_data/batch_X`
- Automatically records **5 batches** (total 50 seconds of data)

---

## 🛠 Hardware Setup
- **ESP32 Development Board**
- **Sensors on ADC1 pins only**  
  ADC1 pins available: **GPIO 32–39**

Default pins:
- Sensor 1 → **GPIO 36**
- Sensor 2 → **GPIO 37**

❗ **Do NOT use ADC2 pins** (GPIO 0, 2, 4, 12–15, 25–27) –  
WiFi uses ADC2 internally, so readings will fail.

---

## ⚙️ Setup Instructions

### 1. Install Required Library
Search for:

