# mqtt_test

This directory contains test scripts for MQTT server.

## Prepare venv

```bash
$ uv sync --frozen
Installed 1 package in 18ms
 + paho-mqtt==2.1.0
```

## Publishing a message

```bash
uv run python -m mqtt_publish.py
```

## Listening for a message

```bash
uv run python -m mqtt_listen.py
```
