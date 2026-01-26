# mqtt_receiver

CLI script for observing all MQTT topics (excluding $SYS/#) and printing payloads.

## Prepare the venv

```bash
uv sync --frozen
```

## Run the receiver

```bash
uv run python -m mqtt_receiver
```

## Configuration

The script reads connection settings from environment variables:

- `MQTT_HOST` (default: `localhost`)
- `MQTT_PORT` (default: `1883`)
- `MQTT_USER` (default: `mqtt-receiver`)
- `MQTT_PASSWORD` (default: `mqtt-receiver-password`)
