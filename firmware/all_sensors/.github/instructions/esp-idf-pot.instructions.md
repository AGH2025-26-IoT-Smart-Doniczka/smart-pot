---
applyTo: '**'
---
Role: Expert Embedded C System Architect specializing in ESP-IDF.

Objective: Develop robust firmware for a "Smart Plant Monitor" on ESP32. The system is a low-power, event-driven IoT device that wakes up, connects to WiFi, gathers sensor data, uploads it (or stores it if offline), and sleeps. It features BLE provisioning and complex button interaction logic.

1. Technical Constraints
Framework: ESP-IDF v5.4.1 (or latest v5.x stable). Use modern components (esp_event, esp_timer, esp_netif).
Language: Strict C (C99/C11). No C++.
Architecture: Event-Driven Finite State Machine (FSM) via esp_event_loop.
Principles: Clean Architecture, Modular Design, SOLID (in C context).
Hardware: ESP32 (Generic/S3/C3).
2. Architecture & Design Patterns
Orchestration: A centralized fsm_manager controls the flow. It listens to events and triggers state transitions.
Decoupling: Modules (WiFi, MQTT, Sensors) never call each other directly. They strictly communicate via events (Observer Pattern).
Example: Sensor task finishes → posts APP_EVENT_SENSORS_DATA_READY → FSM hears it → FSM decides next step.
States: Flat FSM structure (No composite states).
Structure:
components/core: Shared types, Event IDs, Enums.
components/managers: Logic (FSM, WiFi, MQTT, BLE, Power, NVS).
components/drivers: Hardware Abstraction (Sensors, BSP).
3. Finite State Machine (FSM) Flow
The system must implement the following app_state_t lifecycle:
STATE_INIT: Check NVS. -> PROVISIONING (if empty) or WIFI_CONNECT (if saved).
STATE_PROVISIONING: BLE GATT Server. Accepts WiFi creds + Plant Config (11-byte binary).
STATE_WIFI_CONNECT: Async connect. Success -> SYNC_TIME. Fail -> SENSING (Offline Fallback).
STATE_SYNC_TIME: SNTP Sync. -> SENSING.
STATE_SENSING: Parallel sensor reading (I2C/ADC). Aggregates data.
STATE_DATA_DECISION: Choice Node. If WiFi connected -> MQTT_PUBLISH. Else -> FLASH_STORE.
STATE_MQTT_PUBLISH: Upload JSON.
STATE_FLASH_STORE: Save to SPI Flash (Ringbuffer).
STATE_DEEP_SLEEP: Low power.
STATE_FACTORY_RESET: Erase NVS & Reboot.
Global Interrupts:
Button 3s: Trigger PROVISIONING.
Button 10s: Trigger FACTORY_RESET.
4. Data Protocols
BLE Config (Write): 11 bytes raw.
Byte 0: Lux target (0-2).
Bytes 1-4: Moisture thresholds (4x uint8).
Bytes 5-8: Temp thresholds (2x uint16, Kelvin * 10).
Bytes 9-10: Sleep duration (uint16 seconds).
potId is its base mac
MQTT Data (Publish): JSON { "potId": "...", "ts": 123456789, "data": { "lux": 1, "tem": 24.5, "moi": 45, "pre": 1013 } }.
wherever there is sensor data is should be of this type // Sensors data
 struct {
    uint16_t lux_level;         // 0/1/2
    uint8_t soil_moisture;      // ADC value or %
    uint16_t temperature;       // dK
    float pressure;             // hPa
} ;
5. Coding Standards
Style: Pure C. English only.
Comments: Strict section headers required:
C
/* =========================================================================
   SECTION: <Section Name>
   ========================================================================= */
Types: Use app_event_id_t and app_state_t exactly as defined in the context. Use stdint.h types.
Memory: Prefer static allocation for core structures. Use cJSON for JSON.
If an argument is provided as a pointer, or data is saved to a provided pointer, the names should indicate the direction of data movement (in/out).
6. Required Output
When generating code, provide production-ready source files (.c and .h) adhering to the structure above. Ensure esp_event is the primary mechanism for logic flow.
7. The states entry cb should run the desired instructions, the exit callback should clean up, free allocated memory etc. especially if the event is global interrupt event in which case it should not emit a new event.
8. Additional knowlage
## BLE UUID

UUID BLE:
service:
    a2909447-7a7f-d8b8-d140-68a237aa735c

    password (write):
        ee0bfcd4-bdce-6898-0c4a-72321e3c6f45
    
    ssid (write):
        88971243-7282-049b-d14a-e951055fc3a3

    config (write):
        2934e2ce-26f9-4705-bd1d-dfb343f63d04

    user_id (read):
        752ff574-058c-4ba3-8310-b6daa639ee4d


# Peripherals
## I2C
Oprogramowanie wykorzystuje 2 szyny I2C - jedna obsługująca ekran, druga obsługująca sensory.
### Ekran
Ekral oled o rozdzielczości 128x64. Korzysta ze sterownika **SSD1306**
Sterownik:

### Sensory
- Cyfrowe:
  - **BMP280** sensor temperatury i ciśnienia 
  - **VEML7700** sensor naświetlenia, obslugiwany autorskim sterownikiem
- Analogowe:
  - pojemnościowy sensor wilgotności gleby

