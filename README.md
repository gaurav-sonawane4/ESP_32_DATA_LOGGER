# ESP32 High-Speed Data Logger (200Hz)

This project logs **two analog sensors at 200Hz (every 5ms)** using an ESP32 hardware timer.  
The data is stored in RAM for **10 seconds**, packed as a compressed CSV string, and uploaded to **Firebase Realtime Database** in batches.

---

## 🚀 Features
- 200Hz sampling using ESP32 hardware timer
- Reads 2 analog channels using `adc1_get_raw` (fast & WiFi-safe)
- Buffers **2000 samples per batch** (10 seconds)
- Uploads to Firebase under:  
  `/sensor_data/batch_X`
- Collects **5 continuous batches** (50 seconds of data)
- Lightweight CSV format reduces Firebase write operations

---

## 🛠 Hardware Setup

### ESP32 Board  
Any ESP32 DevKit or equivalent.

### Sensor Pins (Very Important)
Use **ADC1 pins only**, because **ADC2 fails when WiFi is ON**.

Valid ADC1 pins: **GPIO 32 – 39**

Default configuration:
- Sensor 1 → **GPIO 36**
- Sensor 2 → **GPIO 37**

❗ **Do NOT use ADC2 pins (0, 2, 4, 12–15, 25–27)**  
They will give random values or NULL when WiFi is active.

---

## ⚙️ Getting Started

### 1️⃣ Install Required Library
Open Arduino Library Manager and install:
