# Smart Healthcare System

## 📌 Project Overview

The **Smart Healthcare System** is an IoT-based healthcare monitoring and patient assistance system developed using an **ESP32**. The system integrates sensors, actuators, FreeRTOS multitasking, OLED display, and **Adafruit IO** cloud monitoring.

The project is designed to monitor healthcare and environmental parameters and provide alerts when predefined conditions are exceeded.

## ⚙️ Technologies Used

* ESP32
* FreeRTOS
* Wokwi Simulator
* Adafruit IO
* MQTT
* Arduino IDE
* OLED Display
* Servo Motor
* Buzzer
* LED
* Analog Sensors

## 🔧 Main Features

* ❤️ Heart-rate monitoring
* 🫁 SpO₂ monitoring
* 🌡️ Body-temperature monitoring
* 🌡️ Room-temperature monitoring
* 💨 Oxygen-level monitoring
* 🌫️ Air Quality Index (AQI) monitoring
* 💊 Medicine dosage control
* 🛏️ Bed-position control
* ⚙️ Adjustable sampling rate
* 🚨 Automatic alert using LED and buzzer
* ☁️ Cloud monitoring using Adafruit IO
* 🔄 Multitasking using FreeRTOS

## 🧠 FreeRTOS Tasks

The system is divided into multiple FreeRTOS tasks:

* `SensorTask` – Reads sensor values
* `ServoTask` – Controls the servo motor
* `OLEDTask` – Updates the OLED display
* `AlertTask` – Handles LED and buzzer alerts
* `MQTTTask` – Sends data to Adafruit IO

## ☁️ Adafruit IO

The system uses Adafruit IO for cloud-based monitoring.

### Feeds Used

* `heart-rate`
* `spo2`
* `body-temp`
* `room-temp`
* `oxygen-level`
* `aqi`
* `dosage-slider`
* `bed-slider`
* `sampling-rate`

### 🔐 Security Note

The **Adafruit IO API key is not included in this repository** for security reasons.

To run the project with your own Adafruit IO account, create your own credentials and configure them in the code.

## 🔌 Hardware

| Component     | Purpose                 |
| ------------- | ----------------------- |
| ESP32         | Main controller         |
| OLED Display  | Local data display      |
| Servo Motor   | Bed/position control    |
| AQI Sensor    | Air-quality monitoring  |
| Oxygen Sensor | Oxygen-level monitoring |
| LED           | Warning indication      |
| Buzzer        | Audible alert           |

## 🔄 System Workflow

```text
Sensors
   ↓
ESP32
   ↓
FreeRTOS Tasks
   ↓
Data Processing
   ↓
OLED Display
   ↓
Adafruit IO / MQTT
   ↓
Cloud Dashboard
```

For abnormal conditions:

```text
Sensor Data
    ↓
Threshold Check
    ↓
Abnormal Condition?
   ↙          ↘
 Yes           No
 ↓             ↓
LED + Buzzer   Normal Operation
```

## ▶️ How to Run

1. Open the project files in Wokwi.
2. Open `sketch.ino`.
3. Configure your own Wi-Fi and Adafruit IO credentials.
4. Add your own Adafruit IO API key locally.
5. Start the Wokwi simulation.
6. Open your Adafruit IO dashboard to monitor the feeds.

> **Never upload your personal Adafruit IO API key to GitHub.**

## 📁 Project Files

* `sketch.ino` – ESP32 source code
* `diagram.json` – Wokwi circuit configuration
* `libraries.txt` – Required libraries
* `wokwi-project.txt` – Wokwi project configuration
* `README.md` – Project documentation

## 👥 Project

**Smart Healthcare System**

Developed as an integrated IoT and embedded-systems project using ESP32, FreeRTOS, MQTT and Adafruit IO.

## 🔗 Wokwi Simulation

[▶️ Open Smart Healthcare System in Wokwi](https://wokwi.com/projects/467786036786179073)
