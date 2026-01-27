# mqtt_sender

CLI script for publishing a single MQTT message to a given topic.

## Prepare the venv

```bash
uv sync --frozen
```

## Run the sender

Publish a preset event:

```bash
uv run python -m mqtt_sender --event telemetry
```

Publish a custom message:

```bash
uv run python -m mqtt_sender --topic "devices/8813BF69B3DA/telemetry" --payload '{"ts": 1710000000.0, "data": {"lux": 123, "tem": 22.5, "moi": 40, "pre": 1010}}'
```

Override QoS/retain:

```bash
uv run python -m mqtt_sender --event logs --qos 0 --retain true
```

## Configuration

The script reads connection settings from environment variables:

- `MQTT_HOST` (default: `localhost`)
- `MQTT_PORT` (default: `1883`)
- `MQTT_USER` (default: `mqtt-sender`)
- `MQTT_PASSWORD` (default: `mqtt-sender-password`)
