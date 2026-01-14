#!/bin/sh
set -e

/opt/venv/bin/python /mosquitto/agent/main.py &

exec mosquitto -c /mosquitto/config/mosquitto.conf
