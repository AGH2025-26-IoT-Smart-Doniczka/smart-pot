# Infra

Container and orchestration assets. Keep Dockerfiles, docker-compose.yml, env samples, and reverse proxy or deployment configs here.

## MQTT

This system uses the Eclipse Mosquitto MQTT broker (Dockerized) and a simple, scalable topic model for each ESP32 device.

## 1. Topics

### ESP -> Server

- `devices/<uuid>/telemetry`
  - ESP publishes sensor telemetry data to the backend.

- `devices/<uuid>/logs`
  - ESP publishes device logs and events.

### Server -> ESP

- `devices/<uuid>/config`
  - Backend publishes configuration updates.
  - ESP updates its internal config upon receiving this payload.

- `devices/<uuid>/actions`
  - Backend publishes actions such as watering.

- `devices/<uuid>/hard-reset`
  - Backend publishes hard reset action.

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
pattern write devices/%u/logs
pattern read  devices/%u/config
pattern read  devices/%u/actions
pattern read  devices/%u/hard-reset
```

This enforces:

- ESP can publish:

    ```text
    devices/<uuid>/telemetry
    devices/<uuid>/logs
    ```

- ESP can subscribe only to:

    ```text
    devices/<uuid>/config
    devices/<uuid>/actions
    devices/<uuid>/hard-reset
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
    String topicLogs      = "devices/" + uuid + "/logs";
    String topicConfig    = "devices/" + uuid + "/config";
    String topicActions   = "devices/" + uuid + "/actions";
    String topicHardReset = "devices/" + uuid + "/hard-reset";
    ```

4. Publish structured JSON payloads.

### Telemetry Payload Example

```json
{
    "ts": 1712345678,
    "data": {
        "lux": 123,
        "tem": 22.5,
        "moi": 42,
        "pre": 1003
    }
}
```

### Logs Payload Example

```json
{
    "ts": 1712345678,
    "lab": "watering",
    "lvl": 2,
    "data": "watering completed"
}
```

### Config Payload Example (Server -> ESP)

```json
{
    "lux": 1,
    "moi": [20, 80],
    "tem": [18.0, 28.5],
    "mes": 300,
    "sen": 300,
    "wat": 3600
}
```

### Actions Payload Example (Server -> ESP)

```json
{
    "typ": "wtr",
    "data": {
        "dur": 10
    }
}
```

## 5. Backend Requirements

The backend:

- Subscribes to:

    ```text
    devices/+/telemetry
    devices/+/logs
    devices/+/hard-reset
    ```

- Publishes config to:

    ```text
    devices/<uuid>/config
    ```

- Publishes actions to:

    ```text
    devices/<uuid>/actions
    ```

## 6. Mosquitto Broker Setup

Broker is based on the official eclipse-mosquitto Docker image.

Includes:

- `mosquitto.conf`
- Dynamic Security plugin
- device-role with ACL patterns
- MQTT listener on 1883
