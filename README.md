🚀 DHT11 & MQ2 Sensor Integration for IoT Projects 🌐

This repository provides comprehensive code and documentation for integrating DHT11 (🌡️ Temperature & Humidity Sensor) and MQ2 (🔥 Gas Sensor) into IoT projects using microcontrollers like ESP32. The data collected from these sensors can be processed locally and transmitted to Edge, Fog, or Cloud systems using protocols like MQTT, HTTP, or CoAP.

✨ Features

🌡️ DHT11 Sensor: Measures temperature and humidity with easy-to-use digital output.

🔥 MQ2 Sensor: Detects gases like LPG, smoke, methane, and propane.

📡 Data Transmission: Supports MQTT, HTTP, and CoAP for sending data to Edge/Fog/Cloud architecture.

🧠 Preprocessing: Filter and transform sensor data before transmission.

🛠️ Hardware Requirements

🧩 DHT11 Sensor Module

🔥 MQ2 Gas Sensor Module

🖥️ ESP32

🔌 Jumper wires and breadboard

⚙️ Setup & Usage

🛠️ Connect the DHT11 and MQ2 sensors to the microcontroller.

🧱 Install necessary libraries for reading sensor data.

🌐 Configure protocols for data transmission (MQTT, HTTP, CoAP).

🚀 Run the script and monitor real-time sensor data.


# Espressif 32: development platform for [PlatformIO](https://platformio.org)

[![Build Status](https://github.com/platformio/platform-espressif32/workflows/Examples/badge.svg)](https://github.com/platformio/platform-espressif32/actions)

ESP32 is a series of low-cost, low-power system on a chip microcontrollers with integrated Wi-Fi and Bluetooth. ESP32 integrates an antenna switch, RF balun, power amplifier, low-noise receive amplifier, filters, and power management modules.

* [Home](https://registry.platformio.org/platforms/platformio/espressif32) (home page in the PlatformIO Registry)
* [Documentation](https://docs.platformio.org/page/platforms/espressif32.html) (advanced usage, packages, boards, frameworks, etc.)

# Usage

1. [Install PlatformIO](https://platformio.org)
2. Create PlatformIO project and configure a platform option in [platformio.ini](https://docs.platformio.org/page/projectconf.html) file:

## Stable version

See `platform` [documentation](https://docs.platformio.org/en/latest/projectconf/sections/env/options/platform/platform.html#projectconf-env-platform) for details.

```ini
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
; build_flags = 
; 	-D ARDUINO_USB_MODE=1
; 	-D ARDUINO_USB_CDC_ON_BOOT=1
lib_deps = 
	ArduinoHttpClient
	ArduinoJson
	DHT20
	PubSubClient
	ThingsBoard
	adafruit/DHT sensor library@^1.4.6
	miguel5612/MQUnifiedsensor@^3.0.0


    
# Configuration

Please navigate to [documentation](https://docs.platformio.org/page/platforms/espressif32.html).

