# 🏠 Offline Smart Home Controller (ESP8266)

A fully autonomous, offline smart home system built on the NodeMCU (ESP8266). This project eliminates cloud dependency by forcefully disabling the Wi-Fi radio, ensuring ultra-stable, low-power, and instant-response local automation.

## ✨ Features

*   **🌡️ Smart PWM Fan Control (Hysteresis):** The fan doesn't just snap on and off. It plays a futuristic "power-up" swoop tone exactly once when triggered, and uses analog PWM (at a silent 25kHz frequency to prevent motor whine) to smoothly ramp up motor speed as the room gets hotter. It includes a 15-point cooling hysteresis lock to prevent sensor jitter.
*   **🚨 IR Security System:** Long-pressing the physical button arms the system and plays a smooth, custom-coded classic Airtel ringtone. If the IR sensor detects motion, it triggers a rapid 6-burst intruder alarm.
*   **🚪 Automated Door System:** Single-pressing the button opens the door via a Servo motor. A 5-second non-blocking timer automatically closes it, preceded by a "Ding-Dong" warning chime.
*   **💡 Smart Lighting:** The main LED activates automatically if the room gets dark (via LDR) or if the temperature rises above the threshold.
*   **🎵 Custom Acoustic Engine:** Uses a custom non-blocking `millis()` and `micros()` tone generator. It includes glide notes (pitch bending) for the fan start-up and a perfectly timed Airtel melody using a single passive buzzer.
*   **🔌 100% Offline:** Wi-Fi is shut down at the hardware level (`WiFi.forceSleepBegin()`) to dedicate 100% of the CPU to real-time sensor reading and audio generation without watchdog resets.

## 🧰 Hardware & Pin Mapping

| Component | NodeMCU Pin | Notes |
| :--- | :--- | :--- |
| **Thermistor** | `A0` (Analog) | Reads ambient room temperature. |
| **LDR** | `D1` (GPIO 5) | Detects room brightness. |
| **IR Sensor** | `D2` (GPIO 4) | Motion detection for the security alarm. |
| **Fan Motor** | `D3` (GPIO 0) | Requires a Transistor/MOSFET. Uses Analog PWM. |
| **Push Button** | `D4` (GPIO 2) | Uses `INPUT_PULLUP`. Connects to GND. |
| **LED** | `D5` (GPIO 14) | System status and automated lighting. |
| **Passive Buzzer** | `D6` (GPIO 12) | Audio engine (Melodies, UI Tones, Alarms). |
| **Servo Motor** | `D7` (GPIO 13) | Controls the automated door mechanism. |

> **Note:** Pin `D3` is specifically chosen for the Fan because it supports smooth hardware PWM, allowing the fan speed to scale with the temperature.

## 🎮 How to Use

1. **Power On:** The system plays a 3-note startup chime, initializes the door servo to the closed position (20 degrees), and takes a baseline temperature reading.
2. **Door Control:** Single-click the button. The buzzer plays a high tone, the door opens, and the 5-second auto-close timer begins.
3. **Security Mode:** Press and hold the button for ~1 second. The system will play the **Airtel Ringtone** and enter Armed Mode. Any motion detected by the IR sensor will trigger the burst alarm. Long press again to disarm.
4. **Climate Control:** Completely automated. Once the temperature exceeds `baseline + 80`, the fan plays a swoop tone and spins up. It will not turn off until the temperature drops 15 points below the threshold.

## 🛠️ Installation & Compilation

1. Open the `.ino` file in the Arduino IDE.
2. Ensure you have the **ESP8266 Board Package** installed.
3. Select **NodeMCU 1.0 (ESP-12E Module)** from the boards menu.
4. No external libraries are required! (Only the built-in `<ESP8266WiFi.h>` and `<Servo.h>`).
5. Compile and upload.

## 🧠 Code Architecture Notes
This project avoids `delay()` wherever possible in the main loop. Sensor polling, fan state management, and the door auto-close timer all run concurrently using `millis()` tracking. The custom `playGlideNote()` function calculates half-periods in real-time to create pitch-bending audio effects without external sound modules.
