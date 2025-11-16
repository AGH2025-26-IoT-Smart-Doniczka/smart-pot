# Infra

Container and orchestration assets. Keep Dockerfiles, docker-compose.yml, env samples, and reverse proxy or deployment configs here.

## MQTT

This system uses the Eclipse Mosquitto MQTT broker (Dockerized) and a simple, scalable three-topic model for each ESP32 device.

## 1. Topics

### ESP -> Server

- `devices/<uuid>/telemetry`
  - ESP publishes sensor telemetry data to the backend.

- `devices/<uuid>/setup`
  - ESP publishes setup-related information.
  - Used for example in pairing requests.

### Server -> ESP

- devices/\<uuid>/config
  - Backend publishes configuration updates.
  - ESP updates its internal config upon receiving this payload.

## 2. Device Identity

Each ESP32 device generates its `<uuid>` from the MAC address.
MAC addresses contain `:` and must be sanitized.

Example: `AA:BB:CC:DD:EE:FF`  ->  `AABBCCDDEEFF`

ESP32 C++ example:

```cpp
uint8_t mac[6];
WiFi.macAddress(mac);

char uuid[13]; // 12 hex chars + null
snprintf(uuid, sizeof(uuid),
            "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
```

## 3. Authentication and Authorization (ACLs)

Each device may only read/write its own topics.

ACL Pattern Rules (with username = `<uuid>`):

```text
pattern write devices/%u/telemetry
pattern write devices/%u/setup
pattern read  devices/%u/config
```

This enforces:

- ESP can publish:

    ```text
    devices/<uuid>/telemetry
    devices/<uuid>/setup
    ```

- ESP can subscribe only to:

    ```text
    devices/<uuid>/config
    ```

- No cross-device access is possible.

### Scalable Management

Use the Mosquitto Dynamic Security Plugin:

- No manual ACL editing
- No broker restarts
- Each device gets:
  - username = \<uuid>
  - password = randomly generated
  - assigned to role "device-role"
- Role contains the ACL patterns above

## 4. ESP32 Firmware Requirements

The ESP32 must:

1. Generate the uuid from MAC (remove `:`)
2. Connect with:
   - clientId = uuid
   - username = uuid
   - password = assigned secret
3. Build topic strings:

    ```text
    String topicTelemetry = "devices/" + uuid + "/telemetry";
    String topicSetup     = "devices/" + uuid + "/setup";
    String topicConfig    = "devices/" + uuid + "/config";
    ```

4. Publish structured JSON payloads.

### Telemetry Payload Example

```json
{
    "type": "telemetry",
    "version": 1,
    "data": {
        "potId": "str",
        "airTemp": 0.0, 
        "airHumidity": 0.0, 
        "airPressure": 0.0, 
        "soilMoisture": 0.0, 
        "illuminance": 0.0
    }
}
```

### Setup Payload Example

```json
{
    "type": "pairing",
    "version": 1,
    "data": {
        "potId": "str",
        "email": "str"
    }
}
```

### Config Payload Example (Server -> ESP)

```json
{
    "type": "config",
    "version": 1,
    "data": {
        "sleepTime": 1800
    }
}
```

## 5. Backend Requirements

The backend:

- Subscribes to:

    ```text
    devices/+/telemetry
    devices/+/setup
    ```

- Publishes config to:

    ```text
    devices/<uuid>/config
    ```

## 6. Mosquitto Broker Setup

Broker is based on the official eclipse-mosquitto Docker image.

Includes:

- `mosquitto.conf`
- Dynamic Security plugin
- device-role with ACL patterns
- MQTT listener on 1883
