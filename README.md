# Pet Feeder Automation System 🐕

A smart pet feeder built with an ESP32, using an Ultrasonic sensor for bowl level detection and a Servo motor for automated food dispensing, configurable via a local Wi-Fi web interface.

## Overview

This project ensures your pet is fed on time, every time. It operates in two primary modes: **Scheduled Feeding**, which dispenses food at pre-set times, and **Auto-Feed**, which detects when the bowl is running low and dispenses a portion, respecting a minimum time interval to prevent overfeeding. The system is managed completely through a local web page.

## Features

* **Time-Based Feeding:** Supports up to **three daily schedules** with precision thanks to NTP (Network Time Protocol) for accurate timekeeping.
* **Level Sensing:** An **Ultrasonic sensor** monitors the distance to the food, calculating the percentage of food left in the bowl.
* **Auto-Refill:** Dispenses food automatically if the bowl level falls below a configurable threshold (e.g., 20%).
* **Anti-Spam Logic:** Prevents continuous dispensing when the bowl is empty by enforcing a minimum interval between auto-feeds.
* **Web Control:** A local web server allows users to set schedules and trigger a **manual dispense**.
* **Servo Control:** A **Servo motor** (SG90 or similar) is used to open and close the food gate/dispenser.

## Hardware Requirements

| Component | Quantity | Notes |
| :--- | :--- | :--- |
| **ESP32** or **ESP8266** | 1 | Microcontroller with built-in Wi-Fi. |
| **Micro Servo Motor (SG90/MG995)** | 1 | For the dispensing mechanism (needs a small chute/gate design). |
| **HC-SR04 Ultrasonic Sensor** | 1 | To measure distance to the food in the bowl. |
| **Jumper Wires** | Various | |
| **Power Supply** | 1 | Stable 5V supply for the ESP32 and Servo (important!). |

## Wiring Diagram

**1. Servo Motor:**
* **VCC** $\rightarrow$ **External 5V** (Highly recommended, Servos draw significant current)
* **GND** $\rightarrow$ **ESP32 GND** (Common Ground)
* **Signal** $\rightarrow$ **ESP32 GPIO 18** (`SERVO_PIN`)

**2. Ultrasonic Sensor (HC-SR04):**
* **VCC** $\rightarrow$ **ESP32 5V**
* **GND** $\rightarrow$ **ESP32 GND**
* **Trig** $\rightarrow$ **ESP32 GPIO 5**
* **Echo** $\rightarrow$ **ESP32 GPIO 17**

## Mechanical Setup Notes

The accuracy of the level sensing relies on the physical mounting:
1.  **Mount the Ultrasonic Sensor** directly above the center of the food bowl, pointing straight down.
2.  **Calibration:** Measure the distance from the sensor to the food surface when the bowl is **full** (`BOWL_FULL_DISTANCE_CM`) and when it is **empty** (`BOWL_EMPTY_DISTANCE_CM`). Update these values in the code for accurate percentage readings.
3.  **Dispenser:** Design a simple chute and gate system controlled by the servo horn to allow food pellets to drop when the servo is at `DISPENSER_OPEN_POS`.

## Software Setup and Installation

1.  **Arduino IDE:** Ensure the ESP32 board core is installed.
2.  **Libraries:**
    * `WiFi.h`, `WebServer.h` (Built-in)
    * `ESP32Servo` (Install via Library Manager)
    * `TimeLib` (Install via Library Manager)
3.  **Configuration:**
    * Update the `ssid` and `password` variables.
    * Set the **Calibration** and **Threshold** variables as detailed above.
    * Adjust the NTP timezone offset in `configTime()` if necessary.
4.  **Upload:** Upload the code to the ESP32.
5.  **Access:** Open the Serial Monitor to find the assigned **IP Address**. Access this IP in a web browser to configure the feeder.
