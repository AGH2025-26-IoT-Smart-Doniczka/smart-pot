# mqtt_test

This directory contains test scripts for the MQTT server.

## Prepare the venv

```bash
$ uv sync --frozen
Installed 1 package in 18ms
 + paho-mqtt==2.1.0
```

## Publishing a message

```bash
uv run python -m mqtt_publish
```

## Listening for a message

```bash
uv run python -m mqtt_listen
```

## Adding a user

```bash
uv run python -m mqtt_add_user
```
