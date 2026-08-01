content = """# 🌿 ESP32 Smart Garden Gateway (IoT Firmware)

![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Framework](https://img.shields.io/badge/Framework-Arduino_Core-teal)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-orange)
![Cloud](https://img.shields.io/badge/Cloud-AWS_IoT_Core-FF9900)

## 📖 Overview
This repository contains the embedded firmware for an ESP32-based IoT Gateway used in a Smart Garden/Home Automation system. The ESP32 acts as the central hub, managing cloud connectivity, a high-performance touch user interface, and delegating real-time hardware execution to an STM32 co-processor.

Built entirely on **FreeRTOS**, this project demonstrates a highly decoupled, dual-core multithreaded architecture.

## ✨ Key Features
*   **🧠 FreeRTOS Multithreading**: Utilizes the ESP32's dual-core processor. The system is divided into 5 independent tasks communicating safely via Queues and Event Groups.
*   **🔒 Secure Cloud Integration**: Connects to AWS IoT Core via MQTTS (Port 8883) using X.509 mutual authentication. Streams telemetry data (Temp, Humid, Currents) and syncs control states in JSON format.
*   **🖥️ High-Performance Touch UI**: Uses `LovyanGFX` with SPI DMA acceleration and PSRAM double-buffering for fluid, tear-free page transitions and gesture recognition on an ST7796 320x480 display.
*   **🔌 Custom Binary UART Protocol**: Implements a robust ESP32 ↔ STM32 communication protocol with frame sync headers (`0xAA 0xBB`) and XOR checksum validation to prevent data corruption.
*   **⚡ Power Management**: Integrates I2C drivers for the AXP2101 PMU and TCA9554 IO expander for stable peripheral power delivery.

## 🏗️ System Architecture

### FreeRTOS Task Breakdown
The firmware Mediator pattern is centered around the `AppManagerTask`, ensuring complete decoupling between UI, Network, and Hardware logic.

1.  **`AppManagerTask` (The Brain)**: Receives events from all sources and routes commands appropriately.
2.  **`UITask`**: Handles rendering and touch inputs. Runs on Core 1 with a large stack to manage PSRAM buffers.
3.  **`WiFiTask`**: Non-blocking state machine to maintain WiFi connection.
4.  **`MQTTTask`**: Manages secure AWS IoT connection and JSON payload packing/parsing.
5.  **`STM32CommTask`**: Serial listener/sender that processes the binary protocol to talk to the STM32 edge device.

## 🛠️ Hardware Requirements
*   **MCU**: ESP32 (WROOM/WROVER with PSRAM heavily recommended)
*   **Co-processor**: STM32 (e.g., STM32F103) for driving Relays and MOSFETs.
*   **Display**: 3.5" ST7796 TFT Display with FT5x06 Capacitive Touch.
*   **PMU & IO**: AXP2101 Power Management Unit, TCA9554 I2C Expander.

## 📂 Project Structure
```text
├── src/
│   ├── main.cpp              # System initialization and Task creation
│   ├── app_manager.cpp       # Central command routing (Mediator)
│   ├── mqtt_manager.cpp      # AWS IoT MQTTS client
│   ├── wifi_manager.cpp      # Auto-reconnect WiFi task
│   ├── uart_protocol.cpp     # ESP32-STM32 Binary UART communication
│   ├── ui.cpp / ui.h         # LovyanGFX GUI rendering and touch logic
│   └── shared_types.h        # Enums, Structs, and Queue handles
├── include/
│   ├── secrets.h             # (REQUIRED) AWS X.509 Certificates and Keys
│   └── logo.c                # Splash screen image map
