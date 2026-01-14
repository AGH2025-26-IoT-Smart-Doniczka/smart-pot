from queue import Queue
from typing import Any, Tuple
import json
from pydantic import ValidationError

from app.schemas.mqtt.pots import (
    TelemetryMqttMessage
)
from app.integrations.repositories.pots import measures_insert


TelemetryEvent = Tuple[str, Any]

telemetry_queue: Queue[TelemetryEvent] = Queue()


def telemetry_handler(topic: str, payload: dict) -> None:
    telemetry_queue.put((topic, payload))

def _normalize_payload(payload: Any) -> dict:
    if isinstance(payload, dict):
        return payload

    if isinstance(payload, (bytes, bytearray)):
        payload = payload.decode()

    if isinstance(payload, str):
        return json.loads(payload)

    raise TypeError(f"Unsupported payload type: {type(payload)}")


def telemetry_worker() -> None:
    while True:
        topic, payload = telemetry_queue.get()

        try:
            raw_payload = _normalize_payload(payload)
        except (json.JSONDecodeError, UnicodeDecodeError, TypeError) as exc:
            print(f"Invalid telemetry payload on {topic}: {exc}")
            continue

        try:
            data = TelemetryMqttMessage(**raw_payload)
        except ValidationError as e:
            print("Validation error", e)
            continue
        else:
            print("unix timestamp:", data.timestamp)
            print("Lux:", data.data.lux)

        pot_id = topic.split("/")[1]
        print("Pot ID:", pot_id)

        try:
            measures_insert(
                pot_id=pot_id,
                timestamp=data.timestamp,
                air_temp=data.data.tem,
                air_pressure=data.data.pre,
                soil_moisture=data.data.moi,
                illuminance=data.data.lux
            )
        except Exception as e:
            print(f"Error inserting telemetry data for pot {pot_id}: {e}")
